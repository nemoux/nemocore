#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <nemotable.h>
#include <nemomisc.h>

struct nemotable *nemotable_create(int cells, int rows, int columns, ...)
{
	struct nemotable *table;
	va_list vargs;
	int offset;
	int i;

	table = (struct nemotable *)malloc(sizeof(struct nemotable));
	if (table == NULL)
		return NULL;
	memset(table, 0, sizeof(struct nemotable));

	table->cells = cells;
	table->rows = rows;
	table->columns = columns;

	table->elements = (int *)malloc(sizeof(int) * columns);
	if (table->elements == NULL)
		goto err1;

	va_start(vargs, columns);

	for (i = 0, offset = 0; i < columns; i++) {
		table->elements[i] = offset;
		offset += ALIGN(va_arg(vargs, int), 4);
	}

	va_end(vargs);

	table->columnsize = offset;
	table->cellsize = rows * table->columnsize;
	table->tablesize = cells * table->cellsize;

	table->buffer = malloc(table->tablesize);
	if (table->buffer == NULL)
		goto err2;

	return table;

err2:
	free(table->elements);

err1:
	free(table);

	return NULL;
}

void nemotable_destroy(struct nemotable *table)
{
	free(table->buffer);
	free(table->elements);
	free(table);
}

int nemotable_set_cell_count(struct nemotable *table, int cells)
{
	table->cells = cells;
	table->tablesize = cells * table->cellsize;

	table->buffer = realloc(table->buffer, table->tablesize);
	if (table->buffer == NULL)
		return -1;

	return 0;
}
