#ifndef __NEMO_TABLE_H__
#define __NEMO_TABLE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemotable {
	int cells;
	int rows;
	int columns;

	int *elements;

	int columnsize;
	int cellsize;
	int tablesize;

	void *buffer;
};

extern struct nemotable *nemotable_create(int cells, int rows, int columns, ...);
extern void nemotable_destroy(struct nemotable *table);

extern int nemotable_set_cell_count(struct nemotable *table, int cells);

static inline void *nemotable_get_element(struct nemotable *table, int cell, int row, int column)
{
	return table->buffer + table->cellsize * cell + table->columnsize * row + table->elements[column];
}

static inline void nemotable_set_iattr(struct nemotable *table, int cell, int row, int column, int value)
{
	int *p = nemotable_get_element(table, cell, row, column);

	*p = value;
}

static inline int nemotable_get_iattr(struct nemotable *table, int cell, int row, int column)
{
	int *p = nemotable_get_element(table, cell, row, column);

	return *p;
}

static inline void nemotable_set_fattr(struct nemotable *table, int cell, int row, int column, float value)
{
	float *p = nemotable_get_element(table, cell, row, column);

	*p = value;
}

static inline float nemotable_get_fattr(struct nemotable *table, int cell, int row, int column)
{
	float *p = nemotable_get_element(table, cell, row, column);

	return *p;
}

static inline void nemotable_set_sattr(struct nemotable *table, int cell, int row, int column, const char *value)
{
	char *p = nemotable_get_element(table, cell, row, column);

	strcpy(p, value);
}

static inline const char *nemotable_get_sattr(struct nemotable *table, int cell, int row, int column)
{
	const char *p = nemotable_get_element(table, cell, row, column);

	return p;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
