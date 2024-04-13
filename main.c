#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <ctype.h>
#include <math.h>


#define NOB_IMPLEMENTATION
#include "nob.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


typedef float Vec2[2];

typedef struct {
	size_t    count;
	Vec2     *positions;
	uint32_t *cons;
	float     total;
} TSP_Info;

// my weird addition to nob.h :)
bool nob_sv_begins_cstr(Nob_String_View sv, const char *cstr)
{
	int l = strlen(cstr);

	if (sv.count < strlen(cstr)) {
		return false;
	}

	for (int i=0; i<l; ++i) {
		if (sv.data[i] != cstr[i]) {
			return false;
		}
	}
	return true;
}

Nob_String_View nob_sv_chop(Nob_String_View *sv, size_t n)
{

	if (sv->count >= n) {
        sv->count -= n;
        sv->data  += n;
	} else {
		sv->data += sv->count;
		sv->count = 0;
	}
}

bool read_tsp(const char *tsp_file, TSP_Info *tsp_out)
{
	Nob_String_Builder content = {0};
	if (!nob_read_entire_file(tsp_file, &content)) {
		printf("read_tsp: error: couldn't read file `%s`\n", tsp_file);
		return false;
	}
	printf("read_tsp: Got %d chars in %s file\n", (int)content.count, tsp_file);

	/* useful functions from nob.h

	const char *nob_temp_sv_to_cstr(Nob_String_View sv);

	Nob_String_View nob_sv_chop_by_delim(Nob_String_View *sv, char delim);
	Nob_String_View nob_sv_trim(Nob_String_View sv);
	bool nob_sv_eq(Nob_String_View a, Nob_String_View b);
	Nob_String_View nob_sv_from_cstr(const char *cstr);
	Nob_String_View nob_sv_from_parts(const char *data, size_t count);
	*/

	Nob_String_View text = nob_sv_from_parts(content.items, content.count);
	text = nob_sv_trim(text);

	// search for DIMENSION: <number>
	const char *dim_str = "DIMENSION";
	while (!nob_sv_begins_cstr(text, dim_str)) {
		nob_sv_chop(&text, 1);
	}
	nob_sv_chop(&text, strlen(dim_str));

	text = nob_sv_trim_left(text);

	if (text.data[0] != ':') {
		printf("read_tsp: error: expected a `:` here: DIMENISON <-\n");
		return false;
	}
	nob_sv_chop(&text, 1); // chop the `:`
	text = nob_sv_trim_left(text);


	// we parse the dimension and malloc the tsp.
	TSP_Info tsp = {0};
	{
		size_t dim=0;
		size_t cur=0;
		while (cur < text.count && isdigit(text.data[cur])) {
			cur++;
		}
		if (cur > 0) {
			Nob_String_View supposed_int = text;
			supposed_int.count = cur;
			dim = (size_t)atoi(nob_temp_sv_to_cstr(supposed_int));
			nob_sv_chop(&text, cur);
		}

		if (dim == 0 || cur == 0) {
			printf("read_tsp: error: failed to parse here: DIMENISON: ... <-\n");
			return false;
		}

		tsp.count = dim;
		tsp.positions = malloc(sizeof(Vec2) * dim);
		tsp.cons = malloc(sizeof(*tsp.cons) * dim);

		assert(tsp.positions != NULL);
		assert(tsp.cons != NULL);

		printf("read_tsp: found %d coordinates\n", (int)dim);
	}

	// chop until first coordinate x: 1 x y
	nob_sv_chop_by_delim(&text, '1');
	text = nob_sv_trim_left(text);

	for (size_t index = 0; index < tsp.count; index++) {
		for (int i=0; i<2; i++) {
			size_t cur=0;
			float result=NAN;
			while (cur < text.count && (isdigit(text.data[cur]) || text.data[cur] == '.')) cur++;
			if (cur > 0) {
				Nob_String_View supposed_float = text;
				supposed_float.count = cur;
				result = (float)atof(nob_temp_sv_to_cstr(supposed_float));
				nob_sv_chop(&text, cur);
			}

			if (isnan(result) || cur == 0) {
				printf("read_tsp: error: failed to parse here: %d ... ... <-\n", (int)index + 1);
				return false;
			}

			tsp.positions[index][i] = result;
			text = nob_sv_trim_left(text);
		}
		// find the next line begining with index ... ...
		nob_sv_chop_by_delim(&text, ' ');
		text = nob_sv_trim_left(text);
	}

	nob_sb_free(content);
	*tsp_out = tsp;
	return true;
}

void normalize_bounds(TSP_Info *tsp)
{
	float min_x = FLT_MAX;
	float max_x = -FLT_MAX;

	float min_y = FLT_MAX;
	float max_y = -FLT_MAX;

	for (size_t i = 0; i < tsp->count; ++i) {
		if (min_x > tsp->positions[i][0]) min_x = tsp->positions[i][0];
		if (max_x < tsp->positions[i][0]) max_x = tsp->positions[i][0];
		if (min_y > tsp->positions[i][1]) min_y = tsp->positions[i][1];
		if (max_y < tsp->positions[i][1]) max_y = tsp->positions[i][1];
	}

	float normalize_factor_x = 1.0f / (max_x - min_x);
	float normalize_factor_y = 1.0f / (max_y - min_y);

	for (size_t i = 0; i < tsp->count; ++i) {
		tsp->positions[i][0] -= min_x;
		tsp->positions[i][0] *= normalize_factor_x;
		tsp->positions[i][1] -= min_y;
		tsp->positions[i][1] *= normalize_factor_y;

	}
}


int generate_usa_picture(void)
{
	const char *file = "test_data/usa13509.tsp";

	TSP_Info tsp = {0};
	read_tsp(file, &tsp);

	normalize_bounds(&tsp);
	{
		//uint32_t width  = 1920;
		//uint32_t height = 1080;
		uint32_t width  = 320;
		uint32_t height = 200;

		const char *filename = "usa.png";

		uint32_t *pixels = malloc(sizeof(*pixels) * width * height);

		// make grey background
		for (size_t i = 0; i < width * height; i++) {
			pixels[i] = 0xFFaaaaaa;
		}

		// draw a pixel per coordinate
		for (size_t i = 0; i < tsp.count; i++) {
			uint32_t x = (width-1) * (1.0 - tsp.positions[i][1]);
			uint32_t y = (height-1) * (1.0 - tsp.positions[i][0]);

			pixels[y*width + x] = (uint32_t)0xFF000000;
		}

		stbi_write_png(filename, width, height, 4, pixels, width*sizeof(*pixels));
	}

	return 0;
}

float dist(TSP_Info *tsp, int i, int j)
{
	float dx = tsp->positions[i][0] - tsp->positions[j][0];
	float dy = tsp->positions[i][1] - tsp->positions[j][1];
	return sqrtf(dx*dx + dy*dy);
}

void init_configuration(TSP_Info *tsp)
{
	tsp->total = 0;
	for (uint32_t i = 0; i < tsp->count; ++i) {
		int next_index = (i + 1) % tsp->count;
		tsp->cons[i] = next_index;
		tsp->total += dist(tsp, i, next_index);
	}
}

// propose a swap of two connections and return the diff in total length
float propose_swap(TSP_Info *tsp, int i, int j)
{
	assert(i >= 0 && j >= 0 && i < tsp->count && j < tsp->count);
	int t1 = tsp->cons[i];
	int t2 = tsp->cons[j];
	float l_prev = dist(tsp, i, t1) + dist(tsp, j, t2);
	float l_new  = dist(tsp, i, t2) + dist(tsp, j, t1);
	return l_new - l_prev;
}

void do_swap(TSP_Info *tsp, int i, int j, float delta_l)
{
	tsp->total += delta_l;
	int t1 = tsp->cons[i];
	int t2 = tsp->cons[j];
	tsp->cons[i] = t2;
	tsp->cons[j] = t1;
}

// fermi dirac
float p(float dist, float temperature)
{
	return 1.0 / (1 + expf(dist/temperature));
}

int main(void)
{
	//const char *file = "test_data/usa13509.tsp";
	const char *file = "test_data/berlin52.tsp";

	TSP_Info tsp = {0};
	read_tsp(file, &tsp);
	init_configuration(&tsp);

	assert(tsp.positions != NULL);
	assert(tsp.cons != NULL);


	const int NSTEPS = 100000000;

	float temperature = 0.01;
	float start_total = tsp.total;
	for (int step = 0; step < NSTEPS; step++) {

		int i = rand()%tsp.count;
		int j = rand()%tsp.count;


		float delta_l  = propose_swap(&tsp, i, j);
		float random_p = (float)rand()/ (float)RAND_MAX;

		if (random_p < p(delta_l, temperature)) {
			// accepted
			do_swap(&tsp, i, j, delta_l);
			//printf("ACCEPTED dist change `%f` (random p=%f) with temperature `%f`\n", delta_l, random_p, temperature);

		} else {
			//printf("Rejected dist change `%f` (random p=%f) with temperature `%f`\n", delta_l, random_p, temperature);
		}


		//printf("Total Distance is: %f\n", tsp.total);
	}

	printf("\n--------\nSUMMARY:\n--------\n\n");
	printf("Start config dist: %f\n", start_total);
	printf("End config dist:   %f\n", tsp.total);
	printf("temperature:       %f\n", temperature);
	printf("number of steps:   %d\n", NSTEPS);
	printf("\n");

	return 0;
}