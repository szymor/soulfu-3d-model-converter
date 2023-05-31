#include <stdio.h>
#include <stdlib.h>

#define MAX_DDD_TEXTURE				(4)
#define MAX_DDD_SHADOW_TEXTURE		(4)
#define DDD_SCALE_WEIGHT			(20000.0f)
#define DDD_EXTERNAL_BONE_FRAMES	(16384)

#define BE_SHORT(b1, b2)	(((unsigned short)(b1) << 8) | (b2))

// buffer for DDD file
unsigned char *ddd = NULL;
size_t ddd_size = 0;

int load_file(char *filename, unsigned char **buff, size_t *size);

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

unsigned char get_rendering_mode(unsigned char *texture);
unsigned short get_triangle_num(unsigned char *texture);
unsigned char *get_triangles(unsigned char *texture);

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

int main(int argc, char *argv[])
{
	printf("SoulFu 3D Model Converter\n\n");

	if (argc < 2)
	{
		printf("No arguments given.\n");
		return 1;
	}

	if (load_file(argv[1], &ddd, &ddd_size) < 0)
	{
		printf("Cannot load the given file.\n");
		return 2;
	}

	float scale = get_scaling(ddd) / DDD_SCALE_WEIGHT;
	printf("Scaling: %.6f\n", scale);
	printf("Header flags: %04x\n", get_header_flags(ddd));
	int base_model_num = get_base_model_num(ddd);
	printf("Number of base models: %d\n", base_model_num);
	int bone_frame_num = get_bone_frame_num(ddd);
	printf("Number of bone frames: %d\n", bone_frame_num);
	char *bff = get_bone_frame_filename(ddd);
	if (bff)
		printf("Bone frame filename: %c%c%c%c%c%c%c%c\n", bff[0], bff[1], bff[2], bff[3], bff[4], bff[5], bff[6], bff[7]);
	unsigned char *base_model = get_first_base_model(ddd);
	for (int i = 0; i < base_model_num; ++i)
	{
		printf("Base model %d\n", i);
		printf("  Number of vertices: %d\n", get_vertex_num(base_model));
		printf("  Number of texture vertices: %d\n", get_texture_vertex_num(base_model));
		printf("  Number of joints: %d\n", get_joint_num(base_model));
		printf("  Number of bones: %d\n", get_bone_num(base_model));
		base_model = get_next_base_model(base_model);
	}

	// print some data about bone frames
	if (!bff)
	{
		unsigned char *bone_frame = base_model;
		for (int i = 0; i < bone_frame_num; ++i)
		{
			printf("Bone frame %d\n", i);
			printf("  Action name: %02x\n", get_action_name(bone_frame));
			printf("  Action modifier flags: %02x\n", get_action_modifier_flags(bone_frame));
			printf("  Base model id: %d\n", get_base_model_id(bone_frame));
			bone_frame = get_next_bone_frame(ddd, bone_frame);
		}
	}

	// convert to OBJ
	char filename[16];
	base_model = get_first_base_model(ddd);
	for (int i = 0; i < base_model_num; ++i)
	{
		sprintf(filename, "model%d.OBJ", i);
		FILE *out = fopen(filename, "w");

		fprintf(out, "# OBJ file generated from SoulFu DDD file %s\n", argv[1]);
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
			vtable += 9;
		}

		// texture vertices
		int texture_vertices = get_texture_vertex_num(base_model);
		unsigned char *tvtable = get_texture_vertices(base_model);
		for (int j = 0; j < texture_vertices; ++j)
		{
			float u = (signed short)BE_SHORT(tvtable[0], tvtable[1]) / 256.0f;
			// note the minus
			float v = -(signed short)BE_SHORT(tvtable[2], tvtable[3]) / 256.0f;
			fprintf(out, "vt %.6f %.6f\n", u, v);
			tvtable += 4;
		}

		// faces
		unsigned char *texture = get_first_texture(base_model);
		for (int j = 0; j < 4; ++j)
		{
			int triangles = get_triangle_num(texture);
			unsigned char *ttable = get_triangles(texture);
			if (triangles > 0)
				fprintf(out, "usemtl material%d\n", j);
			for (int k = 0; k < triangles; ++k)
			{
				fprintf(out, "f %d/%d %d/%d %d/%d\n", BE_SHORT(ttable[0], ttable[1]) + 1, BE_SHORT(ttable[2], ttable[3]) + 1,
					BE_SHORT(ttable[4], ttable[5]) + 1, BE_SHORT(ttable[6], ttable[7]) + 1,
					BE_SHORT(ttable[8], ttable[9]) + 1, BE_SHORT(ttable[10], ttable[11]) + 1);
				ttable += 12;
			}

			texture = get_next_texture(texture);
		}

		fclose(out);
		base_model = get_next_base_model(base_model);
	}

	free(ddd);
	return 0;
}

int load_file(char *filename, unsigned char **buff, size_t *size)
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

unsigned char get_rendering_mode(unsigned char *texture)
{
	return texture[0];
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
