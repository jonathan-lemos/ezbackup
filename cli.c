/* cli.c -- cli dialog/menu
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "log.h"
#include <ncurses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>

static void print_in_middle(WINDOW* win, int rows, int cols, const char* msg){
	char* tmp;
	char* tmp_tok = NULL;
	size_t n_lines = 0;
	size_t cur_line;
	size_t i;

	tmp = malloc(strlen(msg) + 1);
	if (!tmp){
		log_enomem();
		return;
	}
	strcpy(tmp, msg);

	for (i = 0; i < strlen(tmp); ++i){
		if (tmp[i] == '\n'){
			n_lines++;
		}
	}
	cur_line = (rows - n_lines) / 2;

	tmp_tok = strtok(tmp, "\n");
	while (tmp_tok != NULL){
		mvwprintw(win, cur_line, (cols - strlen(tmp_tok)) / 2, tmp_tok);
		cur_line++;
		tmp_tok = strtok(NULL, "\n");
	}

	free(tmp);
}

int display_dialog(const char* const* choices, int n_choices, const char* msg){
	WINDOW* win_body = NULL;
	WINDOW* win_msg = NULL;
	WINDOW* win_menu = NULL;
	WINDOW* win_menu_sub = NULL;
	ITEM** items = NULL;
	MENU* menu = NULL;
	int row, col;
	int c;
	int i;
	size_t len_strings = 0;
	int res;

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	curs_set(0);
	getmaxyx(stdscr, row, col);

	win_body = newwin(row, col, 0, 0);
	box(win_body, 0, 0);

	win_msg = derwin(win_body, row - 5, col - 4, 1, 2);
	box(win_msg, 0, 0);

	print_in_middle(win_msg, row - 5, col - 4, msg);

	win_menu = derwin(win_body, 3, row - 4, col - 4, 2);
	box(win_menu, 0, 0);
	keypad(win_menu, TRUE);

	items = malloc(sizeof(*items) * (n_choices + 1));
	if (!items){
		log_enomem();
		res = -1;
		goto cleanup;
	}
	for (i = 0; i < n_choices; ++i){
		items[i] = new_item(choices[i], "");
		len_strings += strlen(choices[i]);
	}
	items[i] = NULL;
	len_strings += n_choices;

	menu = new_menu(items);

	set_menu_win(menu, win_menu);
	win_menu_sub = derwin(win_menu, 1, len_strings, 1, (col - 4 - len_strings) / 2);
	set_menu_sub(menu, win_menu_sub);
	set_menu_format(menu, 1, n_choices);
	set_menu_mark(menu, "");

	post_menu(menu);

	refresh();
	wrefresh(win_body);
	wrefresh(win_msg);
	wrefresh(win_menu);

	while((c = wgetch(win_menu)) != '\n'){
		ITEM* cur_item;
		switch (c){
		case KEY_LEFT:
			cur_item = current_item(menu);
			res = item_index(cur_item);
			if (res <= 0){
				menu_driver(menu, REQ_LAST_ITEM);
			}
			else{
				menu_driver(menu, REQ_LEFT_ITEM);
			}
			break;
		case KEY_RIGHT:
			cur_item = current_item(menu);
			res = item_index(cur_item);
			if (res >= n_choices){
				menu_driver(menu, REQ_FIRST_ITEM);
			}
			else{
				menu_driver(menu, REQ_RIGHT_ITEM);
			}
		}
	}
	res = item_index(current_item(menu));

cleanup:
	curs_set(1);
	menu ? unpost_menu(menu) : 0;
	menu ? free_menu(menu) : 0;
	for (i = 0; items && i < n_choices; ++i){
		free_item(items[i]);
	}
	free(items);
	win_menu_sub ? delwin(win_menu_sub) : 0;
	win_menu ? delwin(win_menu) : 0;
	win_msg ? delwin(win_msg) : 0;
	win_body ? delwin(win_body) : 0;
	endwin();
	return res;
}

int display_menu(const char* const* options, int num_options, const char* title){
	ITEM** my_items = NULL;
	int c;
	MENU* my_menu = NULL;
	WINDOW* my_menu_window = NULL;
	WINDOW* my_menu_subwindow = NULL;
	int i;
	int row, col;
	int ret;

	if (!options || !title || num_options <= 0){
		log_enull();
		ret = -1;
		goto cleanup;
	}

	/* initialize curses */
	initscr();
	/* disable line buffering for stdin, so our output
	 * shows up immediately */
	cbreak();
	/* disable echoing of our characters */
	noecho();
	/* enable arrow keys */
	keypad(stdscr, TRUE);

	/* disable cursor blinking */
	curs_set(0);
	/* get coordinates of current terminal */
	getmaxyx(stdscr, col, row);

	/* create item list */
	my_items = malloc((num_options + 1) * sizeof(ITEM*));
	for (i = 0; i < num_options; ++i){
		my_items[i] = new_item(options[i], NULL);
	}
	my_items[num_options] = NULL;

	/* initialize menu and windows */
	my_menu = new_menu(my_items);
	my_menu_window = newwin(col - 4, row - 4, 2, 2);
	my_menu_subwindow = derwin(my_menu_window, col - 12, row - 12, 3, 3);
	keypad(my_menu_window, TRUE);

	/* attach menu to our subwindow */
	set_menu_win(my_menu, my_menu_window);
	set_menu_sub(my_menu, my_menu_subwindow);

	/* change selected menu item mark */
	set_menu_mark(my_menu, "> ");

	clear();
	/* put a box around window */
	box(my_menu_window, 0, 0);
	/* display our title */
	mvwprintw(my_menu_window, 1, (row - 6 - strlen(title)) / 2, title);
	/* put another around the title */
	mvwaddch(my_menu_window, 2, 0, ACS_LTEE);
	mvwhline(my_menu_window, 2, 1, ACS_HLINE, row - 6);
	mvwaddch(my_menu_window, 2, row - 5, ACS_RTEE);
	refresh();

	/* post the menu */
	post_menu(my_menu);
	wrefresh(my_menu_window);

	/* while the user does not press enter */
	while ((c = wgetch(my_menu_window)) != '\n'){
		ITEM* cur_item;
		/* handle menu input */
		switch(c){
		case KEY_DOWN:
			cur_item = current_item(my_menu);
			ret = item_index(cur_item);
			if (ret >= num_options - 1){
				menu_driver(my_menu, REQ_FIRST_ITEM);
			}
			else{
				menu_driver(my_menu, REQ_DOWN_ITEM);
			}
			break;
		case KEY_UP:
			cur_item = current_item(my_menu);
			ret = item_index(cur_item);
			if (ret <= 0){
				menu_driver(my_menu, REQ_LAST_ITEM);
			}
			else{
				menu_driver(my_menu, REQ_UP_ITEM);
			}
			break;
		}
	}
	ret = item_index(current_item(my_menu));

cleanup:
	curs_set(1);
	my_menu ? unpost_menu(my_menu) : 0;
	my_menu ? free_menu(my_menu) : 0;
	for (i = 0; my_items && i < num_options; ++i){
		free_item(my_items[i]);
	}
	free(my_items);
	my_menu_subwindow ? delwin(my_menu_subwindow) : 0;
	my_menu_window ? delwin(my_menu_window) : 0;
	endwin();

	return ret;
}
