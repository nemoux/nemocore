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

	int *elementsizes;
	int *elementoffsets;

	int rowsize;
	int cellsize;
	int tablesize;

	void *buffer;
};

extern struct nemotable *nemotable_create(int maximum_elements);
extern void nemotable_destroy(struct nemotable *table);

extern void nemotable_set_element_size(struct nemotable *table, int index, int size);
extern void nemotable_set_column_count(struct nemotable *table, int columns);
extern void nemotable_set_row_count(struct nemotable *table, int rows);
extern void nemotable_set_cell_count(struct nemotable *table, int cells);

extern int nemotable_update(struct nemotable *table);

static inline void *nemotable_get_element(struct nemotable *table, int cell, int row, int column)
{
	return table->buffer + table->cellsize * cell + table->rowsize * row + table->elementoffsets[column];
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

static inline int nemotable_has_iattr(struct nemotable *table, int cell, int row, int column, int value)
{
	int *p = nemotable_get_element(table, cell, row, column);

	return *p == value;
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

static inline int nemotable_has_fattr(struct nemotable *table, int cell, int row, int column, float value)
{
	float *p = nemotable_get_element(table, cell, row, column);

	return *p == value;
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

static inline int nemotable_has_sattr(struct nemotable *table, int cell, int row, int column, const char *value)
{
	char *p = nemotable_get_element(table, cell, row, column);

	return strcmp(p, value) == 0;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
