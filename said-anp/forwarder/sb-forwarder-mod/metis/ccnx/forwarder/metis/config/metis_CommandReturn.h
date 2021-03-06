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
 * @file metis_CommandReturn.h
 * @brief The return code used by CLI commands
 *
 * This return code is used throughout the command parser and command implementations
 * to indicate success, failure, or if the program should exit.
 *
 */

#ifndef Metis_metis_command_return_h
#define Metis_metis_command_return_h

/**
 * @typedef MetisControlReturn
 * @abstract A command returns one of (SUCCESS, FAILURE, EXIT)
 * @constant SUCCESS means the command succeeded
 * @constant FAILURE indicates failure
 * @constant EXIT means the command indicated that metis-control should exit.
 * @discussion <#Discussion#>
 */
typedef enum metis_command_return {
    MetisCommandReturn_Success,      // command returned success
    MetisCommandReturn_Failure,      // command failure
    MetisCommandReturn_Exit          // command indicates program should exit
} MetisCommandReturn;

#endif // Metis_metis_command_return_h
