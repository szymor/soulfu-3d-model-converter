#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_DDD_TEXTURE				(4)
#define MAX_DDD_SHADOW_TEXTURE		(4)
#define DDD_SCALE_WEIGHT			(20000.0f)
#define DDD_EXTERNAL_BONE_FRAMES	(16384)
#define JOINT_COLLISION_SCALE		(0.015f)

#define RENDER_LIGHT_FLAG			(1)
#define RENDER_COLOR_FLAG			(2)
#define RENDER_NOCULL_FLAG			(4)
#define RENDER_ENVIRO_FLAG			(8)
#define RENDER_CARTOON_FLAG			(16)
#define RENDER_EYE_FLAG				(32)
#define RENDER_NO_LINE_FLAG			(64)
#define RENDER_PAPER_FLAG			(128)

#define BE_SHORT(b1, b2)	(((unsigned short)(b1) << 8) | (b2))

enum ErrCode
{
	EC_NONE,
	EC_NOARGS,
	EC_NOFILE,
	EC_WRERR,
	EC_NOOP
};

const char *action_strings[] = {
	"boning",
	"stand",
	"walk",
	"stun_begin",
	"stun",
	"stun_end",
	"knock_out_begin",
	"knock_out",
	"knock_out_stun",
	"knock_out_end",
	"bash_left",
	"bash_right",
	"thrust_left",
	"thrust_right",
	"slash_left",
	"slash_right",
	"attack_fail",
	"block_begin",
	"block",
	"block_end",
	"jump_begin",
	"jump",
	"jump_end",
	"ride",
	"swim",
	"swim_forward",
	"magic",
	"fire_begin",
	"fire_ready",
	"fire",
	"fire_end",
	"extra",
	"special_0",
	"special_1",
	"special_2",
	"special_3",
	"special_4",
	"special_5",
	"special_6",
	"special_7",
	"special_8",
	"special_9",
	"special_10",
	"special_11",
	"special_12",
	"double_begin",
	"double",
	"double_end"
};

// buffer for DDD file
unsigned char *ddd = NULL;
size_t ddd_size = 0;

int load_file(const char *filename, unsigned char **buff, size_t *size);

unsigned short get_scaling(unsigned char *ddd);
unsigned short get_header_flags(unsigned char *ddd);
unsigned char get_base_model_num(unsigned char *ddd);
unsigned short get_bone_frame_num(unsigned char *ddd);
unsigned char *get_shadow_textures(unsigned char *ddd);
unsigned char *get_bone_frame_filename(unsigned char *ddd);

unsigned short get_vertex_num(unsigned char *base_model);
unsigned short get_texture_vertex_num(unsigned char *base_model);
unsigned short get_joint_num(unsigned char *base_model);
unsigned short get_bone_num(unsigned char *base_model);
unsigned char *get_vertices(unsigned char *base_model);
unsigned char *get_texture_vertices(unsigned char *base_model);

unsigned char *get_first_base_model(unsigned char *ddd);
unsigned char *get_next_base_model(unsigned char *curr_base_model);

unsigned char *get_first_texture(unsigned char *base_model);
unsigned char *get_next_texture(unsigned char *curr_texture);

unsigned char *get_joint_data(unsigned char *base_model);
unsigned char *get_bone_data(unsigned char *base_model);

unsigned char get_rendering_mode(unsigned char *texture);
unsigned char get_texture_flags(unsigned char *texture);
unsigned char get_texture_alpha(unsigned char *texture);
unsigned short get_triangle_num(unsigned char *texture);
unsigned char *get_triangles(unsigned char *texture);

unsigned char *get_first_bone_frame(unsigned char *ddd);
unsigned char *get_next_bone_frame(unsigned char *ddd, unsigned char *curr_bone_frame);

unsigned char get_action_name(unsigned char *bone_frame);
unsigned char get_action_modifier_flags(unsigned char *bone_frame);
unsigned char get_base_model_id(unsigned char *bone_frame);
unsigned char *get_xy_movement_offset(unsigned char *bone_frame);
unsigned char *get_bones(unsigned char *bone_frame);
unsigned char *get_joints(unsigned char *ddd, unsigned char *bone_frame);
unsigned char *get_shadow_texture_data(unsigned char *ddd, unsigned char *bone_frame);

unsigned char get_shadow_texture_alpha(unsigned char *shadow_texture_data_entry);

unsigned char *get_base_model_from_id(unsigned char *ddd, unsigned char id);

char *get_texture_flag_string(unsigned char flags);

int ddd_to_obj(const char *path);
int obj_to_ddd(char *path);

void fwrite_byte(FILE *file, unsigned char byte);
void fwrite_short(FILE *file, unsigned short word);

char *read_triplet(char *string, int *a, int *b, int *c);

int main(int argc, char *argv[])
{
	printf("SoulFu 3D Model Converter\n\n");

	if (argc < 2)
	{
		printf("No arguments given.\n\n");
		printf("How to use?\n");
		printf("  %s <filename>    convert DDD or OBJ file\n", argv[0]);
		return EC_NOARGS;
	}

	const char *ext = NULL;

	ext = strstr(argv[1], ".obj");
	if (!ext) ext = strstr(argv[1], ".OBJ");
	if (ext && ext[4] == 0)
		return obj_to_ddd(argv[1]);

	ext = strstr(argv[1], ".ddd");
	if (!ext) ext = strstr(argv[1], ".DDD");
	if (ext && ext[4] == 0)
		return ddd_to_obj(argv[1]);

	printf("No operation deduced from the arguments.\n");
	return EC_NOOP;
}

int ddd_to_obj(const char *path)
{
	printf("DDD to OBJ.\n");

	if (load_file(path, &ddd, &ddd_size) < 0)
	{
		printf("Cannot load the file.\n");
		return EC_NOFILE;
	}

	float scale = get_scaling(ddd) / DDD_SCALE_WEIGHT;
	printf("Scaling: %.6f\n", scale);
	int header_flags = get_header_flags(ddd);
	printf("Header flags: %04x\n", header_flags);
	int base_model_num = get_base_model_num(ddd);
	printf("Number of base models: %d\n", base_model_num);
	int bone_frame_num = get_bone_frame_num(ddd);
	printf("Number of bone frames: %d\n", bone_frame_num);

	unsigned char *st = get_shadow_textures(ddd);
	printf("Shadow texture indices: %d %d %d %d\n", st[0], st[1], st[2], st[3]);

	char *bff = get_bone_frame_filename(ddd);
	if (bff)
		printf("Bone frame filename: %c%c%c%c%c%c%c%c\n", bff[0], bff[1], bff[2], bff[3], bff[4], bff[5], bff[6], bff[7]);

	unsigned char *base_model = get_first_base_model(ddd);
	for (int i = 0; i < base_model_num; ++i)
	{
		printf("Base model %d:\n", i);
		printf("  Number of vertices: %d\n", get_vertex_num(base_model));
		printf("  Number of texture vertices: %d\n", get_texture_vertex_num(base_model));
		printf("  Number of joints: %d\n", get_joint_num(base_model));
		printf("  Number of bones: %d\n", get_bone_num(base_model));
		base_model = get_next_base_model(base_model);
	}

	// convert to OBJ
	char filename[16];
	base_model = get_first_base_model(ddd);
	for (int i = 0; i < base_model_num; ++i)
	{
		sprintf(filename, "model%d.OBJ", i);
		FILE *out = fopen(filename, "w");
		if (!out)
		{
			printf("Cannot create %s file.\n", filename);
			free(ddd);
			return EC_WRERR;
		}

		fprintf(out, "# OBJ file generated from SoulFu DDD file %s\n", path);
		fprintf(out, "#  Scaling: %.6f\n", scale);
		fprintf(out, "#  Flags: %04x\n", header_flags);
		if (bff)
			fprintf(out, "#  Bone frame filename: %c%c%c%c%c%c%c%c\n",
				bff[0], bff[1], bff[2], bff[3], bff[4], bff[5], bff[6], bff[7]);

		fprintf(out, "#  Number of vertices: %d\n", get_vertex_num(base_model));

		fprintf(out, "mtllib materials.mtl\n");

		// vertices
		int vertices = get_vertex_num(base_model);
		unsigned char *vtable = get_vertices(base_model);
		for (int j = 0; j < vertices; ++j)
		{
			float x = (signed short)BE_SHORT(vtable[0], vtable[1]) * scale;
			float y = (signed short)BE_SHORT(vtable[2], vtable[3]) * scale;
			float z = (signed short)BE_SHORT(vtable[4], vtable[5]) * scale;
			fprintf(out, "v %.6f %.6f %.6f\n", x, y, z);

			// bone bindings
			fprintf(out, "# bone binding %d %d\n", vtable[6], vtable[7]);

			// bone weighting
			unsigned char anchor = vtable[8] & 0x80;
			unsigned char weight = vtable[8];
			// get rid of anchor flag, just like in render_bone_frame in render.c
			// done regardless anchor flag state
			weight <<= 1;
			fprintf(out, "# bone weighting %.6f, anchor %d\n", weight / 255.0f, anchor ? 1 : 0);

			vtable += 9;
		}

		// texture vertices
		int texture_vertex_num = get_texture_vertex_num(base_model);
		fprintf(out, "# Number of texture vertices: %d\n", texture_vertex_num);
		unsigned char *tvtable = get_texture_vertices(base_model);
		for (int j = 0; j < texture_vertex_num; ++j)
		{
			float u = (signed short)BE_SHORT(tvtable[0], tvtable[1]) / 256.0f;
			// note the minus
			float v = -(signed short)BE_SHORT(tvtable[2], tvtable[3]) / 256.0f;
			fprintf(out, "vt %.6f %.6f\n", u, v);
			tvtable += 4;
		}

		// faces
		unsigned char *texture = get_first_texture(base_model);
		for (int j = 0; j < MAX_DDD_TEXTURE; ++j)
		{
			int triangles = get_triangle_num(texture);
			unsigned char *ttable = get_triangles(texture);
			if (triangles > 0)
			{
				fprintf(out, "# Texture %d\n", j);
				fprintf(out, "#  Rendering mode: %02x\n", get_rendering_mode(texture));
				fprintf(out, "#  Flags: %s\n", get_texture_flag_string(get_texture_flags(texture)));
				fprintf(out, "#  Alpha: %d\n", get_texture_alpha(texture));
				fprintf(out, "#  Number of triangles: %d\n", triangles);
				fprintf(out, "usemtl material%d\n", j);
			}
			for (int k = 0; k < triangles; ++k)
			{
				fprintf(out, "f %d/%d %d/%d %d/%d\n", BE_SHORT(ttable[0], ttable[1]) + 1, BE_SHORT(ttable[2], ttable[3]) + 1,
					BE_SHORT(ttable[4], ttable[5]) + 1, BE_SHORT(ttable[6], ttable[7]) + 1,
					BE_SHORT(ttable[8], ttable[9]) + 1, BE_SHORT(ttable[10], ttable[11]) + 1);
				ttable += 12;
			}

			texture = get_next_texture(texture);
		}

		// joints
		int joint_num = get_joint_num(base_model);
		fprintf(out, "# Number of joints: %d\n", joint_num);
		unsigned char *joints = texture;
		for (int j = 0; j < joint_num; ++j)
		{
			fprintf(out, "#  Joint %d, size %.6f\n", j, joints[j] * JOINT_COLLISION_SCALE);
		}

		// bones
		int bone_num = get_bone_num(base_model);
		fprintf(out, "# Number of bones: %d\n", bone_num);
		unsigned char *bones = get_bone_data(base_model);
		for (int j = 0; j < bone_num; ++j)
		{
			unsigned char bone_id = bones[0];
			unsigned short bjoints[2];
			bjoints[0] = BE_SHORT(bones[1], bones[2]);
			bjoints[1] = BE_SHORT(bones[3], bones[4]);
			fprintf(out, "#  Bone %d, id %d, joints %d %d\n", j, bone_id, bjoints[0], bjoints[1]);
			bones += 5;
		}

		fclose(out);
		printf("Base model %d written to %s.\n", i, filename);
		base_model = get_next_base_model(base_model);
	}

	// add bone frame data to obj models
	if (!bff)
	{
		unsigned char *bone_frame = get_first_bone_frame(ddd);
		for (int i = 0; i < bone_frame_num; ++i)
		{
			int base_model_id = get_base_model_id(bone_frame);
			sprintf(filename, "model%d.OBJ", base_model_id);
			FILE *out = fopen(filename, "a");
			if (!out)
			{
				printf("Cannot append to %s file.\n", filename);
				free(ddd);
				return EC_WRERR;
			}

			fprintf(out, "\n# Bone frame %d\n", i);
			unsigned char action_id = get_action_name(bone_frame);
			fprintf(out, "#  Action name: %s (%02x)\n", action_strings[action_id], action_id);
			fprintf(out, "#  Action modifier flags: %02x\n", get_action_modifier_flags(bone_frame));
			unsigned char *xymo = get_xy_movement_offset(bone_frame);
			float xy_movement_offset[2];
			xy_movement_offset[0] = (signed short)BE_SHORT(xymo[0], xymo[1]) / 256.0f;
			xy_movement_offset[1] = (signed short)BE_SHORT(xymo[2], xymo[3]) / 256.0f;
			fprintf(out, "#  XY movement offset: %.6f, %.6f\n", xy_movement_offset[0], xy_movement_offset[1]);

			unsigned char *bones = get_bones(bone_frame);
			int bone_num = get_bone_num(get_base_model_from_id(ddd, base_model_id));
			for (int j = 0; j < bone_num; ++j)
			{
				float x = (signed short)BE_SHORT(bones[0], bones[1]);
				float y = (signed short)BE_SHORT(bones[2], bones[3]);
				float z = (signed short)BE_SHORT(bones[4], bones[5]);
				float distance = sqrt(x*x + y*y + z*z);
				x /= distance;
				y /= distance;
				z /= distance;
				fprintf(out, "#  Bone %d forward normal: %.6f, %.6f, %.6f\n", j, x, y, z);
				bones += 6;
			}

			unsigned char *joints = get_joints(ddd, bone_frame);
			int joint_num = get_joint_num(get_base_model_from_id(ddd, base_model_id));
			for (int j = 0; j < joint_num; ++j)
			{
				float x = (signed short)BE_SHORT(joints[0], joints[1]) * scale;
				float y = (signed short)BE_SHORT(joints[2], joints[3]) * scale;
				float z = (signed short)BE_SHORT(joints[4], joints[5]) * scale;
				fprintf(out, "#  Joint %d: %.6f, %.6f, %.6f\n", j, x, y, z);
				joints += 6;
			}

			unsigned char *shadow_texture_data = get_shadow_texture_data(ddd, bone_frame);
			for (int j = 0; j < MAX_DDD_SHADOW_TEXTURE; ++j)
			{
				int alpha = get_shadow_texture_alpha(shadow_texture_data);
				if (alpha)
				{
					fprintf(out, "#  Shadow texture %d\n", j);
					fprintf(out, "#   Alpha: %d\n", alpha);
					// vertices
					for (int k = 0; k < 4; ++k)
					{
						float u = (signed short)BE_SHORT(shadow_texture_data[1], shadow_texture_data[2]) * scale;
						float v = (signed short)BE_SHORT(shadow_texture_data[3], shadow_texture_data[4]) * scale;
						fprintf(out, "#   Vertex %d: X %.6f, Y %.6f\n", k, u, v);
					}
					shadow_texture_data += 17;
				}
				else
				{
					++shadow_texture_data;
				}
			}

			bone_frame = get_next_bone_frame(ddd, bone_frame);
			fclose(out);
		}
	}

	free(ddd);
	return EC_NONE;
}

int obj_to_ddd(char *path)
{
	printf("OBJ to DDD.\n");

	printf("Input file: %s\n", path);
	FILE *in = fopen(path, "r");

	// construct the output path
	size_t len = strlen(path);
	path[len - 3] = 'D';
	path[len - 2] = 'D';
	path[len - 1] = 'D';
	const char *outpath = strrchr(path, '/');
	if (outpath)
		++outpath;
	else
		outpath = path;
	printf("Output file: %s\n", outpath);
	FILE *out = fopen(outpath, "wb");

	// ===> write header
	float scale = 0.001f;
	fwrite_short(out, (unsigned short)(scale * DDD_SCALE_WEIGHT));	// scale
	fwrite_short(out, 0xbfff);	// flags
	fwrite_byte(out, 0);	// padding
	fwrite_byte(out, 1);	// number of base models
	fwrite_short(out, 1);	// number of bone frames

	// shadow texture indices
	for (int i = 0; i < MAX_DDD_SHADOW_TEXTURE; ++i)
		fwrite_byte(out, 0);

	// no external bone frame file, no write

	// ===> write a single base model
	long int vtv_offset = ftell(out);
	fwrite_short(out, 0);	// number of vertices - placeholder
	fwrite_short(out, 0);	// number of texture vertices - placeholder
	fwrite_short(out, 2);	// number of joints
	fwrite_short(out, 1);	// number of bones

	// loop over vertices
	int vertex_num = 0;
	fseek(in, 0, SEEK_SET);
	while (!feof(in))
	{
		float x, y, z;
		if (fscanf(in, "v %f %f %f", &x, &y, &z) == 3)
		{
			// coordinates
			fwrite_short(out, (signed short)(x / scale));
			fwrite_short(out, (signed short)(y / scale));
			fwrite_short(out, (signed short)(z / scale));
			// bone binding
			fwrite_byte(out, 0);
			fwrite_byte(out, 0);
			// bone weighting
			fwrite_byte(out, (unsigned char)(0.5f * 255.0f) >> 1);

			++vertex_num;
		}

		// start at a new line
		int ch = 0;
		while ('\n' != ch && EOF != ch)
		{
			ch = fgetc(in);
		}
	}

	// loop over texture vertices
	int texture_vertex_num = 0;
	fseek(in, 0, SEEK_SET);
	while (!feof(in))
	{
		float x, y;
		if (fscanf(in, "vt %f %f", &x, &y) == 2)
		{
			// coordinates
			fwrite_short(out, (signed short)(x * 256.0f));
			fwrite_short(out, (signed short)(-y * 256.0f));

			++texture_vertex_num;
		}

		// start at a new line
		int ch = 0;
		while ('\n' != ch && EOF != ch)
		{
			ch = fgetc(in);
		}
	}
	// write a dummy texture vertex if none exist
	if (0 == texture_vertex_num)
	{
		// coordinates
		fwrite_short(out, (signed short)(0 * 256.0f));
		fwrite_short(out, (signed short)(-0 * 256.0f));
		++texture_vertex_num;
	}

	// textures
	long int face_num_offset[MAX_DDD_TEXTURE] = { 0, 0, 0, 0 };
	int face_num[MAX_DDD_TEXTURE] = { 0, 0, 0, 0 };
	int current_texture_idx = 0;
	char texture_started = 0;
	fseek(in, 0, SEEK_SET);
	// loop over faces
	while (!feof(in))
	{
		int ix, itx, iy, ity, iz, itz;
		char cname[160];
		fscanf(in, "%s", cname);
		if (!strcmp(cname, "f"))
		{
			fscanf(in, "%[^\n]", cname);
			cname[strlen(cname) + 1] = '\0'; // double null ends the string

			char *endptr = NULL;
			endptr = read_triplet(cname, &ix, &itx, NULL);
			if (0 == itx) itx = 1;
			endptr = read_triplet(endptr, &iy, &ity, NULL);
			if (0 == ity) ity = 1;
			endptr = read_triplet(endptr, &iz, &itz, NULL);
			if (0 == itz) itz = 1;

			if (!texture_started)
			{
				fwrite_byte(out, 1);	// rendering mode on
				fwrite_byte(out, 0);	// flags
				fwrite_byte(out, 255);	// alpha
				face_num_offset[current_texture_idx] = ftell(out);
				fwrite_short(out, 0);	// number of faces - placeholder
				texture_started = 1;
			}

			// three vertex - texture vertex pairs
			fwrite_short(out, ix - 1);
			fwrite_short(out, itx - 1);
			fwrite_short(out, iy - 1);
			fwrite_short(out, ity - 1);
			fwrite_short(out, iz - 1);
			fwrite_short(out, itz - 1);

			++face_num[current_texture_idx];

			// check for n-gons where n > 3
			// convex only
			while (iz > 0)
			{
				iy = iz;
				ity = itz;
				endptr = read_triplet(endptr, &iz, &itz, NULL);
				if (0 == itz) itz = 1;
				if (iz > 0)
				{
					// three vertex - texture vertex pairs
					fwrite_short(out, ix - 1);
					fwrite_short(out, itx - 1);
					fwrite_short(out, iy - 1);
					fwrite_short(out, ity - 1);
					fwrite_short(out, iz - 1);
					fwrite_short(out, itz - 1);

					++face_num[current_texture_idx];
				}
			}
		}
		else if (!strcmp(cname, "usemtl"))
		{
			if (texture_started && current_texture_idx < (MAX_DDD_TEXTURE - 1))
			{
				++current_texture_idx;
				texture_started = 0;
			}
		}

		// start at a new line
		int ch = 0;
		while ('\n' != ch && EOF != ch)
		{
			ch = fgetc(in);
		}
	}

	// rendering mode off for unused textures
	for (int i = current_texture_idx + texture_started; i < MAX_DDD_TEXTURE; ++i)
	{
		fwrite_byte(out, 0);
	}

	// joints
	fwrite_byte(out, 0.0f / JOINT_COLLISION_SCALE);
	fwrite_byte(out, 0.0f / JOINT_COLLISION_SCALE);

	// bones
	fwrite_byte(out, 0);	// bone id
	fwrite_short(out, 1);
	fwrite_short(out, 0);

	// ===> write a single bone frame
	fwrite_byte(out, 0);	// action name (0 = boning)
	fwrite_byte(out, 0);	// action modifier flags
	fwrite_byte(out, 0);	// base model id
	fwrite_short(out, 0);	// X movement offset
	fwrite_short(out, 0);	// Y movement offset

	// 1 bone forward normal
	fwrite_short(out, 0);	// X
	fwrite_short(out, -1);	// Y
	fwrite_short(out, 0);	// Z

	// 2 joints
	fwrite_short(out, 0);	// X
	fwrite_short(out, 0);	// Y
	fwrite_short(out, (signed short)(1.0f / scale));	// Z

	fwrite_short(out, 0);	// X
	fwrite_short(out, 0);	// Y
	fwrite_short(out, (signed short)(2.0f / scale));	// Z

	// shadow texture data (alpha only)
	for (int i = 0; i < MAX_DDD_SHADOW_TEXTURE; ++i)
		fwrite_byte(out, 0);

	// fill out placeholders
	for (int i = 0; i < MAX_DDD_TEXTURE; ++i)
	{
		if (face_num_offset[i] != 0)
		{
			fseek(out, face_num_offset[i], SEEK_SET);
			fwrite_short(out, face_num[i]);
		}
	}
	fseek(out, vtv_offset, SEEK_SET);
	fwrite_short(out, vertex_num);
	fwrite_short(out, texture_vertex_num);

	fclose(out);
	fclose(in);
	return EC_NONE;
}

int load_file(const char *filename, unsigned char **buff, size_t *size)
{
	FILE *input = fopen(filename, "rb");
	if (NULL == input)
	{
		return -1;
	}

	fseek(input, 0, SEEK_END);
	*size = ftell(input);

	if (*buff) free(*buff);
	*buff = (unsigned char *)malloc(*size);
	if (*buff == NULL)
	{
		fclose(input);
		return -2;
	}

	fseek(input, 0, SEEK_SET);
	fread(*buff, *size, 1, input);
	fclose(input);
	return 0;
}

unsigned short get_scaling(unsigned char *ddd)
{
	return BE_SHORT(ddd[0], ddd[1]);
}

unsigned short get_header_flags(unsigned char *ddd)
{
	return BE_SHORT(ddd[2], ddd[3]);
}

unsigned char get_base_model_num(unsigned char *ddd)
{
	return ddd[5];
}

unsigned short get_bone_frame_num(unsigned char *ddd)
{
	return BE_SHORT(ddd[6], ddd[7]);
}

unsigned char *get_shadow_textures(unsigned char *ddd)
{
	return ddd + 8;
}

unsigned char *get_bone_frame_filename(unsigned char *ddd)
{
	unsigned short flags = get_header_flags(ddd);
	if (flags & DDD_EXTERNAL_BONE_FRAMES)
		return ddd + 8 + MAX_DDD_SHADOW_TEXTURE;
	else
		return NULL;
}

unsigned short get_vertex_num(unsigned char *base_model)
{
	return BE_SHORT(base_model[0], base_model[1]);
}

unsigned short get_texture_vertex_num(unsigned char *base_model)
{
	return BE_SHORT(base_model[2], base_model[3]);
}

unsigned short get_joint_num(unsigned char *base_model)
{
	return BE_SHORT(base_model[4], base_model[5]);
}

unsigned short get_bone_num(unsigned char *base_model)
{
	return BE_SHORT(base_model[6], base_model[7]);
}

unsigned char *get_vertices(unsigned char *base_model)
{
	return base_model + 8;
}

unsigned char *get_texture_vertices(unsigned char *base_model)
{
	int vertices = get_vertex_num(base_model);
	return base_model + 8 + vertices * 9;
}

unsigned char *get_first_base_model(unsigned char *ddd)
{
	unsigned short flags = get_header_flags(ddd);
	return ddd + 8 + MAX_DDD_SHADOW_TEXTURE + ((flags & DDD_EXTERNAL_BONE_FRAMES) ? 8 : 0);
}

unsigned char *get_next_base_model(unsigned char *curr_base_model)
{
	int joints = get_joint_num(curr_base_model);
	int bones = get_bone_num(curr_base_model);
	unsigned char *ptr = get_first_texture(curr_base_model);
	// we get a pointer after the last texture
	for (int i = 0; i < MAX_DDD_TEXTURE; ++i)
	{
		ptr = get_next_texture(ptr);
	}
	ptr += joints + bones * 5;
	return ptr;
}

unsigned char *get_first_texture(unsigned char *base_model)
{
	int vertices = get_vertex_num(base_model);
	int texture_vertices = get_texture_vertex_num(base_model);
	return base_model + 8 + vertices * 9 + texture_vertices * 4;
}

unsigned char *get_next_texture(unsigned char *curr_texture)
{
	if (get_rendering_mode(curr_texture))
	{
		int triangles = get_triangle_num(curr_texture);
		return curr_texture + 5 + triangles * 3 * 4;
	}
	else
	{
		return curr_texture + 1;
	}
}

unsigned char *get_joint_data(unsigned char *base_model)
{
	unsigned char *ptr = get_first_texture(base_model);
	for (int i = 0; i < MAX_DDD_TEXTURE; ++i)
		ptr = get_next_texture(ptr);
	return ptr;
}

unsigned char *get_bone_data(unsigned char *base_model)
{
	int joints = get_joint_num(base_model);
	return get_joint_data(base_model) + joints;
}

unsigned char get_rendering_mode(unsigned char *texture)
{
	return texture[0];
}

unsigned char get_texture_flags(unsigned char *texture)
{
	// valid only if rendering mode != 0
	return texture[1];
}

unsigned char get_texture_alpha(unsigned char *texture)
{
	// valid only if rendering mode != 0
	return texture[2];
}

unsigned short get_triangle_num(unsigned char *texture)
{
	if (get_rendering_mode(texture))
		return BE_SHORT(texture[3], texture[4]);
	else
		return 0;
}

unsigned char *get_triangles(unsigned char *texture)
{
	if (get_rendering_mode(texture))
		return texture + 5;
	else
		return NULL;
}

unsigned char *get_first_bone_frame(unsigned char *ddd)
{
	int base_model_num = get_base_model_num(ddd);
	unsigned char *ptr = get_first_base_model(ddd);
	for (int i = 0; i < base_model_num; ++i)
	{
		ptr = get_next_base_model(ptr);
	}
	return ptr;
}

unsigned char *get_next_bone_frame(unsigned char *ddd, unsigned char *curr_bone_frame)
{
	unsigned char *ptr = get_shadow_texture_data(ddd, curr_bone_frame);
	for (int i = 0; i < MAX_DDD_SHADOW_TEXTURE; ++i)
	{
		if (get_shadow_texture_alpha(ptr) > 0)
			ptr += 1 + 4 * 4;
		else
			++ptr;
	}
	return ptr;
}

unsigned char get_action_name(unsigned char *bone_frame)
{
	return bone_frame[0];
}

unsigned char get_action_modifier_flags(unsigned char *bone_frame)
{
	return bone_frame[1];
}

unsigned char get_base_model_id(unsigned char *bone_frame)
{
	return bone_frame[2];
}

unsigned char *get_xy_movement_offset(unsigned char *bone_frame)
{
	return bone_frame + 3;
}

unsigned char *get_bones(unsigned char *bone_frame)
{
	return bone_frame + 7;
}

unsigned char *get_joints(unsigned char *ddd, unsigned char *bone_frame)
{
	int bones = get_bone_num(get_base_model_from_id(ddd, get_base_model_id(bone_frame)));
	return get_bones(bone_frame) + bones * 6;
}

unsigned char *get_shadow_texture_data(unsigned char *ddd, unsigned char *bone_frame)
{
	int joints = get_joint_num(get_base_model_from_id(ddd, get_base_model_id(bone_frame)));
	return get_joints(ddd, bone_frame) + joints * 6;
}

unsigned char get_shadow_texture_alpha(unsigned char *shadow_texture_data_entry)
{
	return shadow_texture_data_entry[0];
}

unsigned char *get_base_model_from_id(unsigned char *ddd, unsigned char id)
{
	unsigned char *ptr = get_first_base_model(ddd);
	while (id--) ptr = get_next_base_model(ptr);
	return ptr;
}

char *get_texture_flag_string(unsigned char flags)
{
	static char buff[256];
	buff[0] = 0;
	if (flags & RENDER_LIGHT_FLAG)
		strcat(buff, "light ");
	if (flags & RENDER_COLOR_FLAG)
		strcat(buff, "color ");
	if (flags & RENDER_NOCULL_FLAG)
		strcat(buff, "nocull ");
	if (flags & RENDER_ENVIRO_FLAG)
		strcat(buff, "enviro ");
	if (flags & RENDER_CARTOON_FLAG)
		strcat(buff, "cartoon ");
	if (flags & RENDER_EYE_FLAG)
		strcat(buff, "eye ");
	if (flags & RENDER_NO_LINE_FLAG)
		strcat(buff, "noline ");
	if (flags & RENDER_PAPER_FLAG)
		strcat(buff, "paper ");
	// remove trailing space
	if (buff[0] != 0)
		buff[strlen(buff) - 1] = 0;
	else
		strcat(buff, "none");
	return buff;
}

void fwrite_byte(FILE *file, unsigned char byte)
{
	fwrite(&byte, 1, 1, file);
}

void fwrite_short(FILE *file, unsigned short word)
{
	unsigned char bytes[2];
	bytes[0] = word >> 8;
	bytes[1] = word & 0xff;
	fwrite(bytes, 2, 1, file);
}

char *read_triplet(char *string, int *a, int *b, int *c)
{
	char *pa, *pb, *pc, temp;

	// omit whitespaces
	while (*string == ' ' || *string == '\t') ++string;

	pa = string;

	// ========= move ptr to the next valid triplet
	// remember, double null ends the whole face line
	while ((*string >= '0' && *string <= '9') || *string == '/') ++string;
	++string;
	// =========

	pb = pa;
	while (*pb >= '0' && *pb <= '9') ++pb;
	temp = *pb;
	*pb = '\0';
	if (temp == '/') ++pb;

	pc = pb;
	while (*pc >= '0' && *pc <= '9') ++pc;
	temp = *pc;
	*pc = '\0';
	if (temp == '/') ++pc;

	if (a) *a = 0;
	if (b) *b = 0;
	if (c) *c = 0;

	if (a) sscanf(pa, "%d", a);
	if (b) sscanf(pb, "%d", b);
	if (c) sscanf(pc, "%d", c);

	return string;
}
