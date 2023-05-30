#include <stdio.h>
#include <stdlib.h>

#define DDD_EXTERNAL_BONE_FRAMES 16384

#define BE_SHORT(b1, b2)	(((unsigned short)(b1) << 8) | (b2))

// buffer for DDD file
unsigned char *ddd = NULL;
size_t ddd_size = 0;

int load_file(char *filename, unsigned char **buff, size_t *size);

unsigned char get_base_model_num(unsigned char *ddd);
unsigned short get_bone_frame_num(unsigned char *ddd);

unsigned short get_vertex_num(unsigned char *base_model);
unsigned short get_texture_vertex_num(unsigned char *base_model);
unsigned short get_joint_num(unsigned char *base_model);
unsigned short get_bone_num(unsigned char *base_model);

unsigned char *get_first_base_model(unsigned char *ddd);
unsigned char *get_next_base_model(unsigned char *curr_base_model);

unsigned char *get_first_texture(unsigned char *base_model);
unsigned char *get_next_texture(unsigned char *curr_texture);

unsigned char get_rendering_mode(unsigned char *texture);
unsigned short get_triangle_num(unsigned char *texture);

int main(int argc, char *argv[])
{
	printf("SoulFu 3D Model Converter\n\n");
	load_file("/home/szymek/repos/soulfu/datadir/STOVE00.DDD", &ddd, &ddd_size);

	int base_model_num = get_base_model_num(ddd);
	printf("Number of base models: %d\n", base_model_num);
	printf("Number of bone frames: %d\n", get_bone_frame_num(ddd));
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

unsigned char get_base_model_num(unsigned char *ddd)
{
	return ddd[5];
}

unsigned short get_bone_frame_num(unsigned char *ddd)
{
	return BE_SHORT(ddd[6], ddd[7]);
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

unsigned char *get_first_base_model(unsigned char *ddd)
{
	unsigned short flags = BE_SHORT(ddd[2], ddd[3]);
	return ddd + 12 + ((flags & DDD_EXTERNAL_BONE_FRAMES) ? 8 : 0);
}

unsigned char *get_next_base_model(unsigned char *curr_base_model)
{
	int joints = get_joint_num(curr_base_model);
	int bones = get_bone_num(curr_base_model);
	unsigned char *ptr = get_first_texture(curr_base_model);
	// we get a pointer after the last texture
	for (int i = 1; i < 5; ++i)
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
