#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotable.h>
#include <nemomisc.h>

struct nemotable *nemotable_create(int maximum_elements)
{
	struct nemotable *table;

	table = (struct nemotable *)malloc(sizeof(struct nemotable));
	if (table == NULL)
		return NULL;
	memset(table, 0, sizeof(struct nemotable));

	table->elementsizes = (int *)malloc(sizeof(int) * maximum_elements);
	if (table->elementsizes == NULL)
		goto err1;
	table->elementoffsets = (int *)malloc(sizeof(int) * maximum_elements);
	if (table->elementoffsets == NULL)
		goto err2;

	return table;

err2:
	free(table->elementsizes);

err1:
	free(table);

	return NULL;
}

void nemotable_destroy(struct nemotable *table)
{
	if (table->buffer != NULL)
		free(table->buffer);

	free(table->elementoffsets);
	free(table->elementsizes);
	free(table);
}

void nemotable_set_element_size(struct nemotable *table, int index, int size)
{
	table->elementsizes[index] = ALIGN(size, 4);
}

void nemotable_set_column_count(struct nemotable *table, int columns)
{
	table->columns = columns;
}

void nemotable_set_row_count(struct nemotable *table, int rows)
{
	table->rows = rows;
}

void nemotable_set_cell_count(struct nemotable *table, int cells)
{
	table->cells = cells;
}

int nemotable_update(struct nemotable *table)
{
	int offset;
	int i;

	for (i = 0, offset = 0; i < table->columns; i++) {
		table->elementoffsets[i] = offset;
		offset += table->elementsizes[i];
	}

	table->rowsize = offset;
	table->cellsize = table->rows * table->rowsize;
	table->tablesize = table->cells * table->cellsize;

	table->buffer = realloc(table->buffer, table->tablesize);
	if (table->buffer == NULL)
		return -1;

	return 0;
}
