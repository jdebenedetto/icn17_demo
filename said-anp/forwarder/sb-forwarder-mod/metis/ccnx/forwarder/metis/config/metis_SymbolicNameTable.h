/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file metis_SymbolicNameTable.h
 * @brief The symbolic name table maps a string name to a connection id
 *
 * When configuring tunnels/connections, the user provides a string name (symbolic name) that they
 * will use to refer to that connection.  The symblic name table translates that symbolic name
 * to a connection id.
 *
 */

#ifndef __Metis__metis_SymbolicNameTable__
#define __Metis__metis_SymbolicNameTable__

struct metis_symblic_name_table;
typedef struct metis_symblic_name_table MetisSymbolicNameTable;

#include <stdbool.h>

/**
 * Creates a symbolic name table
 *
 * Allocates a MetisSymbolicNameTable, which will store the symbolic names
 * in a hash table.
 *
 * @retval non-null An allocated MetisSymbolicNameTable
 * @retval null An error
 *
 * Example:
 * @code
 * <#example#>
 * @endcode
 */
MetisSymbolicNameTable *metisSymbolicNameTable_Create(void);

/**
 * Destroys a name table
 *
 * All keys and data are released.
 *
 * @param [in,out] tablePtr A pointer to a MetisSymbolicNameTable, which will be NULL'd
 *
 * Example:
 * @code
 * <#example#>
 * @endcode
 */
void metisSymbolicNameTable_Destroy(MetisSymbolicNameTable **tablePtr);

/**
 * Tests if the name (case insensitive) is in the table
 *
 * Does a case-insensitive match to see if the name is in the table
 *
 * @param [in] table An allocated MetisSymbolicNameTable
 * @param [in] symbolicName The name to check for
 *
 * @retval true The name is in the table
 * @retval false The name is not in the talbe
 *
 * Example:
 * @code
 * <#example#>
 * @endcode
 */
bool metisSymbolicNameTable_Exists(MetisSymbolicNameTable *table, const char *symbolicName);

/**
 * Adds a (name, connid) pair to the table.
 *
 * The name is stored case insensitive.  The value UINT_MAX is used to indicate a
 * non-existent key, so it should not be stored as a value in the table.
 *
 * @param [in] table An allocated MetisSymbolicNameTable
 * @param [in] symbolicName The name to save (will make a copy)
 * @param [in] connid The connection id to associate with the name
 *
 * @retval true The pair was added
 * @retval false The pair was not added (likely duplicate key)
 *
 * Example:
 * @code
 * <#example#>
 * @endcode
 */
bool metisSymbolicNameTable_Add(MetisSymbolicNameTable *table, const char *symbolicName, unsigned connid);

/**
 * Returns the connection id associated with the symbolic name
 *
 * This function will look for the given name (case insensitive) and return the
 * corresponding connid.  If the name is not in the table, the function will return UINT_MAX.
 *
 * @param [in] table An allocated MetisSymbolicNameTable
 * @param [in] symbolicName The name to retrieve
 *
 * @retval UINT_MAX symbolicName not found
 * @retval number the corresponding connid.
 *
 * Example:
 * @code
 * <#example#>
 * @endcode
 */
unsigned metisSymbolicNameTable_Get(MetisSymbolicNameTable *table, const char *symbolicName);

void metisSymbolicNameTable_Remove(MetisSymbolicNameTable *table, const char *symbolicName);

#endif /* defined(__Metis__metis_SymbolicNameTable__) */
