/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "c_private.h"

/* Variable structure */
typedef struct {
        CNamed named;
        CVarType type;
        int size;
        const char *comment;
        bool archive;
        union {
                char *nc;
                short *ns;
                int *n;
                float *f;
                double *fd;
                char *s;
                void *v;
        } p;
        union {
                int n;
                float f;
                char s[0];
        } stock;
} Variable;

/* Variable linked list */
static CNamed *varRoot;

/******************************************************************************\
 Cleanup registered variables.
\******************************************************************************/
void C_cleanupVars(void)
{
        CNamed_free(&varRoot, NULL);
}

/******************************************************************************\
 Register a variable for dynamic configuration.
\******************************************************************************/
void C_register(void *ptr, const char *name, const char *comment,
                CVarType type, int size, bool archive)
{
        Variable *var;
        int memSize;

        /* Allocate memory for variable information and stock value */
        memSize = size > sizeof (var->stock) ? size - sizeof (var->stock) : 0;
        var = CNamed_get(&varRoot, name, sizeof (Variable) + memSize);

        /* Overwrote an old name */
        if (var->p.v)
                C_error("Overwrote variable '%s'", name);

        var->type = type;
        var->p.v = ptr;
        var->size = size;
        var->archive = archive;
        var->comment = comment;

        /* Save "stock" value */
        if (type == CVT_INT)
                var->stock.n = *var->p.n;
        else if (type == CVT_FLOAT)
                var->stock.f = *var->p.f;
        else if (type == CVT_STRING)
                C_strncpy(var->stock.s, ptr, size);
        else
                C_warning("Invalid type %d", type);
}

/******************************************************************************\
 Set a variable's value.
\******************************************************************************/
static void Variable_set(Variable *var, const char *value)
{
        if (var->type == CVT_STRING)
                C_strncpy(var->p.s, value, var->size);
        else if (var->type == CVT_INT) {
                if (var->size == sizeof (char))
                        *var->p.nc = (char)atoi(value);
                else if (var->size == sizeof (short))
                        *var->p.ns = (short)atoi(value);
                else if (var->size == sizeof (int))
                        *var->p.n = atoi(value);
                else
                        C_error("Unsupported variable '%s' size %d",
                                var->named.name, var->size);
        } else if (var->type == CVT_FLOAT) {
                if (var->size == sizeof (float))
                        *var->p.f = (float)atof(value);
                else if (var->size == sizeof (double))
                        *var->p.fd = atof(value);
                else
                        C_error("Unsupported variable '%s' size %d",
                                var->named.name, var->size);
        }
        C_debug("Variable '%s' set to '%s'", var->named.name, value);
}

/******************************************************************************\
 Set a variable's value by name.
\******************************************************************************/
void C_setVar(const char *name, const char *value)
{
        Variable *var;

        /* Get the variable */
        if (!(var = CNamed_get(&varRoot, name, 0))) {
                C_warning("Variable '%s' not defined", name);
                return;
        }

        /* Set the value */
        Variable_set(var, value);
}

/******************************************************************************\
 Parse a configuration command-line.
\******************************************************************************/
void C_parseCommandLine(int argc, char *argv[])
{
        int i, len;
        const char *key, *value;
        char buf[C_NAME_MAX];

        for (i = 1; i < argc; i++) {

                /* Ignore leading dashes */
                for (key = argv[i]; *key == '-'; key++);

                /* Split key=value pairs */
                if ((value = strstr(argv[i], "="))) {
                        if ((len = value++ - key) > sizeof (buf) - 1)
                                len = sizeof (buf) - 1;
                        strcpy(buf, key);
                        buf[len] = NUL;
                        key = buf;
                } else if (i == argc - 1) {
                        C_warning("Variable '%s' has no value", key);
                        break;
                } else
                        value = argv[++i];

                /* Set the value */
                C_setVar(key, value);
        }
}

/******************************************************************************\
 Writes a configuration file that contains the values of all archivable
 variables.
\******************************************************************************/
void C_archiveVars(const char *filename)
{
        Variable *var;
        FILE *file;
        const char *value;

        if (!(file = C_fopen_write(filename)))
                return;
        fprintf(file, "########################################"
                      "########################################\n"
                      "# " PACKAGE_STRING " - Automatically generated config\n"
                      "########################################"
                      "########################################\n");
        for (var = (Variable *)varRoot; var;
             var = (Variable *)var->named.next) {
                if (!var->archive)
                        continue;

                /* Get the variable's value */
                value = NULL;
                switch (var->type) {
                case CVT_INT:
                        if (*var->p.n == var->stock.n)
                                continue;
                        value = C_va("%d", *var->p.n);
                        break;
                case CVT_FLOAT:
                        if (*var->p.f == var->stock.f)
                                continue;
                        value = C_va("%g", *var->p.f);
                        break;
                case CVT_STRING:
                        if (!strcmp(var->p.s, var->stock.s))
                                continue;
                        value = C_escape(var->p.s);
                        break;
                default:
                        C_error("Variable '%s' has invalid type %d",
                                var->named.name, var->type);
                }

                /* Print the comment and key-value pair */
                fprintf(file, "\n# %s\n%s %s\n",
                        var->comment ? var->comment : "",
                        var->named.name, value);
        }
        fprintf(file, "\n");
        fclose(file);
        C_debug("Saved config '%s'", filename);
}

/******************************************************************************\
 Parse a variable configuration file.
\******************************************************************************/
void C_parseVarConfig(const char *filename)
{
        Variable *var;
        FILE *file;
        const char *key;

        if (!(file = C_fopen_read(filename)))
                return;
        C_debug("Parsing variable config '%s'", filename);
        while (!feof(file)) {
                if (!(key = C_token(file)) || !key[0])
                        continue;
                if (!(var = CNamed_get(&varRoot, key, 0))) {
                        C_warning("Unknown variable '%s'", key);
                        continue;
                }
                Variable_set(var, C_token(file));
        }
        fclose(file);
}

