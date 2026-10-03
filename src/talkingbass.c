#define _GNU_SOURCE
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"

#define URI_BASE "https://opencode.local/lv2/talkingbass"
#define URI_SUAVE URI_BASE
#define URI_GROWL URI_BASE "#growl"
#define URI_WHISPER URI_BASE "#whisper"

enum {
	P_OUT_L = 0, P_OUT_R, P_MIDI,
	P_VOWEL, P_LFO_HZ, P_LFO_AMT, P_SPLIT_HZ, P_DRIVE, P_MIX, P_BODY,
	P_COUNT
};

#define NVOICES 8

typedef struct {
	float b0, b1, b2, a1, a2;
	float z1, z2;
} Biquad;

typedef struct {
	int active;
	int note;
	float vel;
	float phase, phase2, phasesub;
	float env, goal;
	float inc, inc2, incsub;
} Voice;

typedef struct {
	float *out_l, *out_r;
	const LV2_Atom_Sequence *midi;
	const float *vowel, *lfo_hz, *lfo_amt, *split_hz, *drive, *mix, *body;
	double sr;
	float lfo_phase;
	int preset;
	Biquad f1[2], f2[2], f3[2];
	Biquad lp[2], hp[2], peak[2];
	float last_f1, last_f2, last_f3, last_split;
	Voice v[NVOICES];
	LV2_URID midi_event;
	LV2_URID atom_blank;
} TalkingBass;

static const float VOWELS[][3] = {
	{730.f, 1090.f, 2440.f},
	{660.f, 1720.f, 2410.f},
	{530.f, 1840.f, 2480.f},
	{570.f,  840.f, 2410.f},
	{300.f,  870.f, 2240.f},
	{270.f, 2290.f, 3010.f},
};

static void bp_set(Biquad *b, float freq, float q, float sr)
{
	if (freq < 40.f) freq = 40.f;
	if (freq > sr * 0.45f) freq = sr * 0.45f;
	if (q < 0.5f) q = 0.5f;
	const float w = 2.f * (float)M_PI * freq / sr;
	const float c = cosf(w);
	const float s = sinf(w);
	const float alpha = s / (2.f * q);
	const float a0 = 1.f + alpha;
	b->b0 = alpha / a0;
	b->b1 = 0.f;
	b->b2 = -alpha / a0;
	b->a1 = -2.f * c / a0;
	b->a2 = (1.f - alpha) / a0;
}

static void lp_set(Biquad *b, float freq, float sr)
{
	if (freq < 20.f) freq = 20.f;
	if (freq > sr * 0.45f) freq = sr * 0.45f;
	const float w = 2.f * (float)M_PI * freq / sr;
	const float c = cosf(w);
	const float s = sinf(w);
	const float alpha = s / (2.f * 0.707f);
	const float a0 = 1.f + alpha;
	b->b0 = (1.f - c) * 0.5f / a0;
	b->b1 = (1.f - c) / a0;
	b->b2 = (1.f - c) * 0.5f / a0;
	b->a1 = -2.f * c / a0;
	b->a2 = (1.f - alpha) / a0;
}

static void hp_set(Biquad *b, float freq, float sr)
{
	if (freq < 20.f) freq = 20.f;
	if (freq > sr * 0.45f) freq = sr * 0.45f;
	const float w = 2.f * (float)M_PI * freq / sr;
	const float c = cosf(w);
	const float s = sinf(w);
	const float alpha = s / (2.f * 0.707f);
	const float a0 = 1.f + alpha;
	b->b0 = (1.f + c) * 0.5f / a0;
	b->b1 = -(1.f + c) / a0;
	b->b2 = (1.f + c) * 0.5f / a0;
	b->a1 = -2.f * c / a0;
	b->a2 = (1.f - alpha) / a0;
}

static void peak_set(Biquad *b, float freq, float q, float gain_db, float sr)
{
	if (freq < 20.f) freq = 20.f;
	if (freq > sr * 0.45f) freq = sr * 0.45f;
	const float A = powf(10.f, gain_db / 40.f);
	const float w = 2.f * (float)M_PI * freq / sr;
	const float c = cosf(w);
	const float s = sinf(w);
	const float alpha = s / (2.f * q);
	const float a0 = 1.f + alpha / A;
	b->b0 = (1.f + alpha * A) / a0;
	b->b1 = -2.f * c / a0;
	b->b2 = (1.f - alpha * A) / a0;
	b->a1 = -2.f * c / a0;
	b->a2 = (1.f - alpha / A) / a0;
}

static float bq(Biquad *b, float x)
{
	const float y = b->b0 * x + b->z1;
	b->z1 = b->b1 * x - b->a1 * y + b->z2;
	b->z2 = b->b2 * x - b->a2 * y;
	return y;
}

static void morph_formants(float v, float *f1, float *f2, float *f3)
{
	if (v < 0.f) v = 0.f;
	if (v > 1.f) v = 1.f;
	const float x = v * 5.f;
	int i = (int)x;
	if (i > 4) i = 4;
	const float t = x - (float)i;
	*f1 = VOWELS[i][0] + (VOWELS[i + 1][0] - VOWELS[i][0]) * t;
	*f2 = VOWELS[i][1] + (VOWELS[i + 1][1] - VOWELS[i][1]) * t;
	*f3 = VOWELS[i][2] + (VOWELS[i + 1][2] - VOWELS[i][2]) * t;
}

static float note_hz(int n)
{
	return 440.f * powf(2.f, (n - 69) / 12.f);
}

static void voice_on(TalkingBass *p, int note, float vel)
{
	Voice *v = NULL;
	for (int i = 0; i < NVOICES; ++i) {
		if (!p->v[i].active) { v = &p->v[i]; break; }
	}
	if (!v) {
		float e = 99.f;
		for (int i = 0; i < NVOICES; ++i) {
			if (p->v[i].env < e) { e = p->v[i].env; v = &p->v[i]; }
		}
	}
	v->active = 1;
	v->note = note;
	v->vel = vel;
	v->goal = 1.f;
	float hz = note_hz(note);
	v->inc = hz / (float)p->sr;
	v->inc2 = hz * 1.0034f / (float)p->sr;
	v->incsub = (hz * 0.5f) / (float)p->sr;
}

static void voice_off(TalkingBass *p, int note)
{
	for (int i = 0; i < NVOICES; ++i) {
		if (p->v[i].active && p->v[i].note == note)
			p->v[i].goal = 0.f;
	}
}

static void handle_midi(TalkingBass *p)
{
	if (!p->midi) return;
	LV2_ATOM_SEQUENCE_FOREACH(p->midi, ev) {
		if (ev->body.type != p->midi_event) continue;
		const uint8_t *m = (const uint8_t *)(ev + 1);
		if (ev->body.size < 1) continue;
		const uint8_t cmd = m[0] & 0xF0;
		if (cmd == 0x90 && ev->body.size >= 3 && m[2] > 0)
			voice_on(p, m[1], m[2] / 127.f);
		else if ((cmd == 0x80 || (cmd == 0x90 && ev->body.size >= 3 && m[2] == 0)) && ev->body.size >= 2)
			voice_off(p, m[1]);
		else if (cmd == 0xB0 && ev->body.size >= 3 && m[1] == 123) {
			for (int i = 0; i < NVOICES; ++i) p->v[i].goal = 0.f;
		}
	}
}

static LV2_Handle instantiate(const LV2_Descriptor *d, double sr,
	const char *bundle, const LV2_Feature *const *features)
{
	(void)bundle;
	TalkingBass *p = (TalkingBass *)calloc(1, sizeof(TalkingBass));
	if (!p) return NULL;
	p->sr = sr;
	p->last_split = -1.f;
	p->preset = 0;
	if (d && d->URI) {
		if (strstr(d->URI, "growl")) p->preset = 1;
		else if (strstr(d->URI, "whisper")) p->preset = 2;
	}
	LV2_URID_Map *map = NULL;
	for (int i = 0; features && features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map))
			map = (LV2_URID_Map *)features[i]->data;
	}
	if (map) {
		p->midi_event = map->map(map->handle, LV2_MIDI__MidiEvent);
		p->atom_blank = map->map(map->handle, LV2_ATOM__Blank);
	}
	return (LV2_Handle)p;
}

static void connect_port(LV2_Handle h, uint32_t port, void *data)
{
	TalkingBass *p = (TalkingBass *)h;
	switch (port) {
	case P_OUT_L: p->out_l = (float *)data; break;
	case P_OUT_R: p->out_r = (float *)data; break;
	case P_MIDI: p->midi = (const LV2_Atom_Sequence *)data; break;
	case P_VOWEL: p->vowel = (const float *)data; break;
	case P_LFO_HZ: p->lfo_hz = (const float *)data; break;
	case P_LFO_AMT: p->lfo_amt = (const float *)data; break;
	case P_SPLIT_HZ: p->split_hz = (const float *)data; break;
	case P_DRIVE: p->drive = (const float *)data; break;
	case P_MIX: p->mix = (const float *)data; break;
	case P_BODY: p->body = (const float *)data; break;
	default: break;
	}
}

static void activate(LV2_Handle h)
{
	TalkingBass *p = (TalkingBass *)h;
	p->lfo_phase = 0.f;
	memset(&p->f1, 0, sizeof(p->f1));
	memset(&p->f2, 0, sizeof(p->f2));
	memset(&p->f3, 0, sizeof(p->f3));
	memset(&p->lp, 0, sizeof(p->lp));
	memset(&p->hp, 0, sizeof(p->hp));
	memset(&p->peak, 0, sizeof(p->peak));
	memset(p->v, 0, sizeof(p->v));
	p->last_f1 = p->last_f2 = p->last_f3 = p->last_split = -1.f;
}

static void run(LV2_Handle h, uint32_t n)
{
	TalkingBass *p = (TalkingBass *)h;
	handle_midi(p);
	float vowel0 = p->vowel ? *p->vowel : (p->preset == 2 ? 0.78f : p->preset == 1 ? 0.52f : 0.18f);
	float lfo_hz = p->lfo_hz ? *p->lfo_hz : (p->preset == 2 ? 1.7f : p->preset == 1 ? 5.8f : 3.4f);
	float lfo_amt = p->lfo_amt ? *p->lfo_amt : (p->preset == 2 ? 0.22f : p->preset == 1 ? 0.46f : 0.28f);
	float split = p->split_hz ? *p->split_hz : (p->preset == 2 ? 145.f : p->preset == 1 ? 118.f : 128.f);
	float drive = p->drive ? *p->drive : (p->preset == 2 ? 0.20f : p->preset == 1 ? 0.52f : 0.36f);
	float mix = p->mix ? *p->mix : 1.f;
	float body = p->body ? *p->body : 0.72f;
	const float sr = (float)p->sr;

	if (fabsf(split - p->last_split) > 0.5f) {
		lp_set(&p->lp[0], split, sr); p->lp[1] = p->lp[0];
		hp_set(&p->hp[0], split, sr); p->hp[1] = p->hp[0];
		peak_set(&p->peak[0], 95.f, 1.1f, 4.5f + body * 3.5f, sr);
		p->peak[1] = p->peak[0];
		p->last_split = split;
	}

	const float att = 1.f - expf(-1.f / (0.008f * sr));
	const float dec = 1.f - expf(-1.f / (0.16f * sr));
	const float rel = 1.f - expf(-1.f / ((p->preset == 2 ? 0.22f : 0.12f) * sr));
	const float sus = p->preset == 2 ? 0.55f : 0.78f;
	const float grit = p->preset == 1 ? 0.22f : p->preset == 2 ? 0.06f : 0.12f;
	const float inc = lfo_hz / sr;

	for (uint32_t i = 0; i < n; ++i) {
		float lfo = 0.5f + 0.5f * sinf(2.f * (float)M_PI * p->lfo_phase);
		p->lfo_phase += inc;
		if (p->lfo_phase >= 1.f) p->lfo_phase -= 1.f;
		float v = vowel0 + (lfo - 0.5f) * 2.f * lfo_amt;
		float f1, f2, f3;
		morph_formants(v, &f1, &f2, &f3);
		if (fabsf(f1 - p->last_f1) > 2.f || fabsf(f2 - p->last_f2) > 2.f) {
			bp_set(&p->f1[0], f1, 7.0f, sr); p->f1[1] = p->f1[0];
			bp_set(&p->f2[0], f2, 6.2f, sr); p->f2[1] = p->f2[0];
			bp_set(&p->f3[0], f3, 5.0f, sr); p->f3[1] = p->f3[0];
			p->last_f1 = f1; p->last_f2 = f2; p->last_f3 = f3;
		}

		float osc = 0.f;
		for (int k = 0; k < NVOICES; ++k) {
			Voice *vc = &p->v[k];
			if (!vc->active && vc->env < 0.0001f) continue;
			if (vc->goal > 0.5f) {
				if (vc->env < sus) vc->env += (1.f - vc->env) * att;
				else vc->env += (sus - vc->env) * dec;
			} else {
				vc->env += (0.f - vc->env) * rel;
				if (vc->env < 0.00015f) { vc->active = 0; vc->env = 0.f; continue; }
			}
			float s1 = 2.f * vc->phase - 1.f;
			float s2 = 2.f * vc->phase2 - 1.f;
			float sq = vc->phase < 0.5f ? 1.f : -1.f;
			float sub = sinf(2.f * (float)M_PI * vc->phasesub);
			osc += vc->vel * vc->env * (0.50f * s1 + 0.28f * s2 + grit * sq + (0.14f + 0.10f * body) * sub);
			vc->phase += vc->inc; if (vc->phase >= 1.f) vc->phase -= 1.f;
			vc->phase2 += vc->inc2; if (vc->phase2 >= 1.f) vc->phase2 -= 1.f;
			vc->phasesub += vc->incsub; if (vc->phasesub >= 1.f) vc->phasesub -= 1.f;
		}

		float yL, yR;
		for (int c = 0; c < 2; ++c) {
			float lo = bq(&p->lp[c], osc);
			lo = bq(&p->peak[c], lo);
			float hi = bq(&p->hp[c], osc);
			float form = bq(&p->f1[c], hi) * 1.45f
				+ bq(&p->f2[c], hi) * 1.10f
				+ bq(&p->f3[c], hi) * 0.48f;
			const float g = 1.f + drive * 3.8f;
			form = tanhf(form * g) / tanhf(g > 1.f ? g : 1.001f);
			float y = lo * (0.85f + 0.25f * body) + mix * form * 1.15f + (1.f - mix) * hi * 0.28f;
			if (c == 0) yL = y; else yR = y;
		}
		if (p->out_l) p->out_l[i] = yL * 0.78f;
		if (p->out_r) p->out_r[i] = yR * 0.78f;
	}
}

static void cleanup(LV2_Handle h) { free(h); }

static const LV2_Descriptor desc0 = { URI_SUAVE, instantiate, connect_port, activate, run, NULL, cleanup, NULL };
static const LV2_Descriptor desc1 = { URI_GROWL, instantiate, connect_port, activate, run, NULL, cleanup, NULL };
static const LV2_Descriptor desc2 = { URI_WHISPER, instantiate, connect_port, activate, run, NULL, cleanup, NULL };

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (index == 0) return &desc0;
	if (index == 1) return &desc1;
	if (index == 2) return &desc2;
	return NULL;
}
