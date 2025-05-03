/*
 Copyright (c) 2025 Mateusz Stadnik

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

extern char **environ;

int main(int argc, char *argv[]);

void _start(int argc, char *argv[]) {
  // Initialize the environment
  environ = (char **)malloc(sizeof(char *));
  environ[0] = 0;

  // Call the main function with the provided arguments
  int ret = main(argc, argv);

  // Exit the program with the return value from main
  exit(ret);
}
