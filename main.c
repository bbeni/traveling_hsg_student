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

//
// ##### CONFIG ######
//

// permutation length for shuffling connections
#define PERM_LEN 8

// number of steps to perform
const int NSTEPS = 1000000;

// linearly decreased together with fermi dirac distr as acceptance probability
// this kinda makes it a simulated annealing approach
float start_temperature = 100.f;

//
// ##### CONFIG ######
//

typedef float Vec2[2];
typedef struct {
	size_t    count;
	Vec2     *positions;
	size_t   *cons;
	float     total;
} TSP_Info;

#define CON_AT(tsp, i) (tsp)->cons[(i) % (tsp)->count]

typedef struct {
	uint32_t *pixels;
	size_t    w, h;
} Image;

#define IMG_AT(img, x, y) img.pixels[y*img.w + x]

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

// normalize the positions to ly in [0, 1] x [0, 1]
void normalize_bounds(Vec2 *positions, size_t count)
{
	float min_x = FLT_MAX;
	float max_x = -FLT_MAX;
	float min_y = FLT_MAX;
	float max_y = -FLT_MAX;

	for (size_t i = 0; i < count; ++i) {
		if (min_x > positions[i][0]) min_x = positions[i][0];
		if (max_x < positions[i][0]) max_x = positions[i][0];
		if (min_y > positions[i][1]) min_y = positions[i][1];
		if (max_y < positions[i][1]) max_y = positions[i][1];
	}

	float normalize_factor_x = 1.0f / (max_x - min_x);
	float normalize_factor_y = 1.0f / (max_y - min_y);

	//printf("[w, h] before normalization [%f, %f]\n", max_x - min_x, max_y - min_y);

	for (size_t i = 0; i < count; ++i) {
		positions[i][0] -= min_x;
		positions[i][0] *= normalize_factor_x;
		positions[i][1] -= min_y;
		positions[i][1] *= normalize_factor_y;
	}
}


void line(Image img, int x0, int y0, int x1, int y1, uint32_t color)
{
	int dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2; /* error value e_xy */

	for (;;) {
		IMG_AT(img, x0, y0) = color;
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
		if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
	}
}

int generate_usa_picture(void)
{
	const char *file = "test_data/usa13509.tsp";

	TSP_Info tsp = {0};
	read_tsp(file, &tsp);

	Vec2 *positions = nob_temp_alloc(sizeof(*tsp.positions) * tsp.count);
	assert(positions != NULL);
	memcpy(positions, tsp.positions, sizeof(*tsp.positions) * tsp.count);

	normalize_bounds(positions, tsp.count);
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
			uint32_t x = (uint32_t) ((width -1) * (1.0-positions[i][1]));
			uint32_t y = (uint32_t) ((height-1) * (1.0-positions[i][0]));

			pixels[y*width + x] = (uint32_t)0xFF000000;
		}

		stbi_write_png(filename, width, height, 4, pixels, width*sizeof(*pixels));
	}

	return 0;
}

void draw_tsp(Image img, TSP_Info *tsp, uint32_t bg, uint32_t pos, uint32_t edge)
{
	Vec2 *positions = nob_temp_alloc(sizeof(Vec2)*tsp->count);
	assert(positions != NULL);
	memcpy(positions, tsp->positions, sizeof(Vec2)*tsp->count);
	normalize_bounds(positions, tsp->count);

	for (size_t i = 0; i < img.w * img.h; i++) {
		img.pixels[i] = bg;
	}

	for (size_t i = 0; i < tsp->count; i++) {
		int next_index = tsp->cons[i];
		uint32_t x =  (uint32_t) ((img.w-1) * (1.0-positions[i][1]));
		uint32_t y =  (uint32_t) ((img.h-1) * (1.0-positions[i][0]));
		uint32_t xn = (uint32_t) ((img.w-1) * (1.0-positions[next_index][1]));
		uint32_t yn = (uint32_t) ((img.h-1) * (1.0-positions[next_index][0]));
		//printf("%d %d %d %d [%d %d]", x, y, xn, yn, img.w, img.h);
		line(img, x, y, xn, yn, edge);
	}

	for (size_t i = 0; i < tsp->count; i++) {
		uint32_t x = (img.w-1) * (1.0 - positions[i][1]);
		uint32_t y = (img.h-1) * (1.0 - positions[i][0]);
		IMG_AT(img, x, y) = pos;
	}
}

void generate_png(const char *fname, TSP_Info tsp, int w, int h, uint32_t bg, uint32_t pos, uint32_t edge)
{
	Image img = {0};
	img.w = w;
	img.h = h;
	img.pixels = malloc(sizeof(img.pixels) * img.w * img.h);
	draw_tsp(img, &tsp, bg, pos, edge);
	stbi_write_png(fname, img.w, img.h, 4, img.pixels, img.w*sizeof(img.pixels));
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

void calc_total(TSP_Info *tsp)
{
	tsp->total = 0;
	for (uint32_t i = 0; i < tsp->count; ++i) {
		int next_index = tsp->cons[i];
		tsp->total += dist(tsp, i, next_index);
	}
}

bool only_one_loop(TSP_Info *tsp)
{
	size_t next = tsp->cons[0];
	for (size_t count = 0; count < tsp->count - 1; count++) {
		if (next == 0) return false;
		//printf("%zd\n", next);
		next = tsp->cons[next];
	}
	return (next == 0);
}

/*// propose a swap of two connections and return the diff in total length
float propose_swap(TSP_Info *tsp, int i, int j)
{
	assert(i >= 0 && j >= 0 && i < tsp->count && j < tsp->count);
	int t1 = tsp->cons[i];
	int t2 = tsp->cons[j];
	if (t1 == j || t2 == i) {
		return FLT_MAX;
	}
	float l_prev = dist(tsp, i, t1) + dist(tsp, j, t2);
	float l_new  = dist(tsp, i, t2) + dist(tsp, j, t1);
	return l_new - l_prev;
}

void do_swap(TSP_Info *tsp, int i, int j)
{
	int t1 = tsp->cons[i];
	int t2 = tsp->cons[j];
	tsp->cons[i] = t2;
	tsp->cons[j] = t1;
}*/

// fermi dirac
float p(float dist, float temperature)
{
	return (float)1.0 / (1 + expf(dist/temperature));
}


//	# # # # # a b c d # # #
//			  2 1 3 0
//	       -> d b a c
// generate a random permutation sequence of length n
//
// ok just use a shuffle :) reference:
// https://stackoverflow.com/questions/6127503/shuffle-array-in-c
//
//

void shuffle_con(TSP_Info *tsp, size_t start, size_t n)
{
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          size_t t = CON_AT(tsp, start + j);
          CON_AT(tsp, start + j) = CON_AT(tsp, start + i);
          CON_AT(tsp, start + i) = t;
        }
    }
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

	generate_png("berlin_random.png", tsp, 1080, 720, 0xFF000000, 0xFFFF0000, 0xFF00FF00);


	float temperature;
	float start_total = tsp.total;
	assert(only_one_loop(&tsp));

	for (int step = 0; step < NSTEPS; step++) {
		//assert(only_one_loop(&tsp));
		temperature = (float)(NSTEPS - step) / (float)NSTEPS * (float)start_temperature;
		size_t shuffle_start = rand()%tsp.count;

		// backup the cons so we can restore if invalid
		int cons_window[PERM_LEN] = {0};

		for (int i = 0; i < PERM_LEN; i++) {
			cons_window[i] = CON_AT(&tsp, shuffle_start + i);
		}

		float total_before = tsp.total;


		shuffle_con(&tsp, shuffle_start, PERM_LEN);

		while (!only_one_loop(&tsp)) {
			shuffle_con(&tsp, shuffle_start, PERM_LEN);
		}
		calc_total(&tsp);

		float delta_l = tsp.total - total_before;
		float random_p = (float)rand()/ (float)RAND_MAX;

		if (delta_l != 0.0f && random_p < p(delta_l, temperature)) {
			printf("%9s: ", nob_temp_sprintf("%zd", step));
			printf("ACCEPTED dist change at temp `%f` with delta `%f` (random p=%f) \n", temperature, delta_l, random_p);
		} else {
			//printf("Rejected dist change `%f` (random p=%f) with temperature `%f`\n", delta_l, random_p, temperature);
			//restore!
			for (int i = 0; i < PERM_LEN; i++) {
				CON_AT(&tsp, shuffle_start + i) = cons_window[i];
			}
			calc_total(&tsp);
		}

		//printf("Total Distance is: %f\n", tsp.total);
	}


	printf("\n--------\nSUMMARY:\n--------\n\n");
	printf("Start config dist: %f\n", start_total);
	printf("End config dist:   %f\n", tsp.total);

	calc_total(&tsp);
	printf("End accurate dist: %f\n", tsp.total);
	printf("temperature:       %f\n", temperature);
	printf("number of steps:   %d\n", NSTEPS);
	printf("\n");
	printf("Tour:\n1");

	for (int i = 0; i < tsp.count; i++) {
		printf(" -> %d", tsp.cons[i] + 1);
	}



	printf("\n\n");
	generate_png("berlin_optimized.png", tsp, 1080, 720, 0xFF000000, 0xFFFF0000, 0xFF00FF00);


	return 0;
}