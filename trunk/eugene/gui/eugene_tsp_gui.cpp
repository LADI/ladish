/* This file is part of Eugene
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Eugene is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Eugene is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Eugene.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <time.h>
#include <gtkmm.h>
#include <glibmm.h>
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Ellipse.hpp>
#include <eugene/Gene.hpp>
#include <eugene/GA.hpp>
#include <eugene/GAImpl.hpp>
#include <eugene/Crossover.hpp>
#include <eugene/Problem.hpp>
#include <eugene/Mutation.hpp>
#include <eugene/TSP.hpp>
#include "MainWindow.hpp"

using namespace std;
using namespace boost;
using namespace FlowCanvas;
using namespace Eugene;
using namespace Eugene::GUI;


int
main(int argc, char** argv)
{
	srand(time(NULL));

	Glib::thread_init();
	Gnome::Canvas::init();

	Gtk::Main main(argc, argv);

	MainWindow* gui = new MainWindow();
	gui->main_win->present();

	main.run(*gui->main_win);

	return 0;
}

