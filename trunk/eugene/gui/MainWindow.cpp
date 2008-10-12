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

#include <eugene/Problem.hpp>
#include <eugene/Selection.hpp>
#include <eugene/OnePointCrossover.hpp>
#include <eugene/PartiallyMappedCrossover.hpp>
#include <eugene/OrderCrossover.hpp>
#include <eugene/PositionBasedCrossover.hpp>
#include <eugene/InjectionCrossover.hpp>
#include <eugene/HybridCrossover.hpp>
#include <eugene/HybridMutation.hpp>
#include <eugene/TournamentSelection.hpp>
#include <eugene/RouletteSelection.hpp>
#include <eugene/RankSelection.hpp>
#include <eugene/TSP.hpp>
#include <eugene/GAImpl.hpp>
#include "MainWindow.hpp"
#include "TSPCanvas.hpp"

using namespace boost;

namespace Eugene {

typedef GAImpl<TSP::GeneType> TSPGA;

namespace GUI {

#define INIT_WIDGET(x) x( xml, #x)

MainWindow::MainWindow()
	: xml(GladeFile::open("eugene_tsp_gui"))
	, INIT_WIDGET(about_dialog)
	, INIT_WIDGET(about_menuitem)
	, INIT_WIDGET(best_generation_label)
	, INIT_WIDGET(crossover_probability_scale)
	, INIT_WIDGET(elites_spin)
	, INIT_WIDGET(execute_button)
	, INIT_WIDGET(generation_label)
	, INIT_WIDGET(greedy_mutation_check)
	, INIT_WIDGET(injection_scale)
	, INIT_WIDGET(main_win)
	, INIT_WIDGET(mutation_scale)
	, INIT_WIDGET(open_menuitem)
	, INIT_WIDGET(order_scale)
	, INIT_WIDGET(partially_mapped_scale)
	, INIT_WIDGET(permute_gene_scale)
	, INIT_WIDGET(population_spin)
	, INIT_WIDGET(position_based_scale)
	, INIT_WIDGET(random_flip_scale)
	, INIT_WIDGET(random_swap_scale)
	, INIT_WIDGET(reverse_scale)
	, INIT_WIDGET(selection_combo)
	, INIT_WIDGET(selection_probability_label)
	, INIT_WIDGET(selection_probability_spin)
	, INIT_WIDGET(shortest_path_label)
	, INIT_WIDGET(single_point_scale)
	, INIT_WIDGET(stop_button)
	, INIT_WIDGET(swap_range_scale)
	, INIT_WIDGET(trial_size_label)
	, INIT_WIDGET(trial_size_spin)
	, INIT_WIDGET(viewport)
{
	selection_combo->set_active(2);
	single_point_scale->set_value(0.0f);
	partially_mapped_scale->set_value(0.1f);
	injection_scale->set_value(0.0f);
	order_scale->set_value(0.8f);
	position_based_scale->set_value(0.1f);
	
	single_point_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_crossover_changed), 0, single_point_scale.get()));
	partially_mapped_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_crossover_changed), 1, partially_mapped_scale.get()));
	injection_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_crossover_changed), 2, injection_scale.get()));
	order_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_crossover_changed), 3, order_scale.get()));
	position_based_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_crossover_changed), 4, position_based_scale.get()));
	
	float crossover_probabilities[] = {
		single_point_scale->get_value(),
		partially_mapped_scale->get_value(),
		injection_scale->get_value(),
		order_scale->get_value(),
		position_based_scale->get_value()
	};
	
	_crossover = shared_ptr< HybridCrossover<TSP::GeneType> >(new HybridCrossover<TSP::GeneType>());
	
	_crossover->append_crossover(crossover_probabilities[0], shared_ptr< Crossover<TSP::GeneType> >(
			new OnePointCrossover<TSP::GeneType>()));

	_crossover->append_crossover(crossover_probabilities[1], shared_ptr< Crossover<TSP::GeneType> >(
			new PartiallyMappedCrossover<TSP::GeneType>()));

	_crossover->append_crossover(crossover_probabilities[2], shared_ptr< Crossover<TSP::GeneType> >(
			new InjectionCrossover<TSP::GeneType>()));
	
	_crossover->append_crossover(crossover_probabilities[3], shared_ptr< Crossover<TSP::GeneType> >(
			new OrderCrossover<TSP::GeneType>()));
	
	_crossover->append_crossover(crossover_probabilities[4], shared_ptr< Crossover<TSP::GeneType> >(
			new PositionBasedCrossover<TSP::GeneType>()));
	
	float mutation_probabilities[] = {
		reverse_scale->get_value(),
		swap_range_scale->get_value(),
		random_swap_scale->get_value(),
		//random_flip_scale->get_value(),
		permute_gene_scale->get_value(),
	};
	
	reverse_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_mutation_changed), 0, reverse_scale.get()));
	swap_range_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_mutation_changed), 1, random_swap_scale.get()));
	random_swap_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_mutation_changed), 2, random_swap_scale.get()));
	/*random_flip_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_mutation_changed), 2, random_flip_scale.get()));*/
	permute_gene_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_mutation_changed), 3, permute_gene_scale.get()));
	
	_mutation = shared_ptr< HybridMutation<TSP::GeneType> >(new HybridMutation<TSP::GeneType>());
	
	_mutation->append_mutation(mutation_probabilities[0], shared_ptr< Mutation<TSP::GeneType> >(
			new ReverseMutation<TSP::GeneType>()));
	
	_mutation->append_mutation(mutation_probabilities[1], shared_ptr< Mutation<TSP::GeneType> >(
			new SwapRangeMutation<TSP::GeneType>()));

	_mutation->append_mutation(mutation_probabilities[2], shared_ptr< Mutation<TSP::GeneType> >(
			new SwapAlleleMutation<TSP::GeneType>()));

	/*_mutation->append_mutation(mutation_probabilities[3], shared_ptr< Mutation<TSP::GeneType> >(
			new RandomMutation<TSP::GeneType>(0, 1)));*/
	
	_mutation->append_mutation(mutation_probabilities[3], shared_ptr< Mutation<TSP::GeneType> >(
			new PermuteMutation<TSP::GeneType>()));
	
	position_based_scale->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this,
			&MainWindow::gtk_on_crossover_changed), 4, position_based_scale.get()));

	elites_spin->signal_changed().connect(sigc::mem_fun(this,
			&MainWindow::gtk_on_num_elites_changed));

	mutation_scale->signal_value_changed().connect(sigc::mem_fun(this,
			&MainWindow::gtk_on_mutation_probability_changed));
	
	crossover_probability_scale->signal_value_changed().connect(sigc::mem_fun(this,
			&MainWindow::gtk_on_crossover_probability_changed));
	
	selection_combo->signal_changed().connect(sigc::mem_fun(this,
			&MainWindow::gtk_on_selection_changed));

	stop_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::gtk_on_stop));
	execute_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::gtk_on_execute));
	
	open_menuitem->signal_activate().connect(sigc::mem_fun(this, &MainWindow::gtk_on_open));
	about_menuitem->signal_activate().connect(sigc::mem_fun(this, &MainWindow::gtk_on_about));
	
	main_win->show_all();
}


void
MainWindow::ga_run()
{
	while (true) {
		const shared_ptr<Eugene::GA> ga(_ga);
		if (ga)
			ga->iteration();
		else
			break;
	}
}


void
MainWindow::gtk_on_crossover_changed(size_t index, const Gtk::Scale* scale)
{
	_crossover->set_probability(index, scale->get_value());
	
	// FIXME: pattern alert
	single_point_scale->set_value(_crossover->crossovers()[0].first);
	partially_mapped_scale->set_value(_crossover->crossovers()[1].first);
	injection_scale->set_value(_crossover->crossovers()[2].first);
	order_scale->set_value(_crossover->crossovers()[3].first);
	position_based_scale->set_value(_crossover->crossovers()[4].first);
}


void
MainWindow::gtk_on_mutation_changed(size_t index, const Gtk::Scale* scale)
{
	static bool enable_signal = true;

	if (!enable_signal)
		return;

	enable_signal = false;

	_mutation->set_probability(index, scale->get_value());
	
	// FIXME: pattern alert
	reverse_scale->set_value(_mutation->mutations()[0].first);
	swap_range_scale->set_value(_mutation->mutations()[1].first);
	random_swap_scale->set_value(_mutation->mutations()[2].first);
	//random_flip_scale->set_value(_mutation->mutations()[3].first);
	permute_gene_scale->set_value(_mutation->mutations()[3].first);

	enable_signal = true;
}
	

void
MainWindow::gtk_on_mutation_probability_changed()
{
	if (_ga)
		_ga->set_mutation_probability(mutation_scale->get_value());
}


void
MainWindow::gtk_on_crossover_probability_changed()
{
	if (_ga)
		_ga->set_crossover_probability(crossover_probability_scale->get_value());
}


void
MainWindow::gtk_on_num_elites_changed()
{
	if (_ga)
		_ga->set_num_elites(elites_spin->get_value());
}

	
void
MainWindow::gtk_on_selection_changed()
{
	switch (selection_combo->get_active_row_number()) {
	case 0:
		trial_size_label->hide();
		trial_size_spin->hide();
		selection_probability_label->hide();
		selection_probability_spin->hide();
		break;
	case 1:
		trial_size_label->hide();
		trial_size_spin->hide();
		selection_probability_label->show();
		selection_probability_spin->show();
		selection_probability_label->set_text("Probability");
		break;
	case 2:
		trial_size_label->show();
		trial_size_spin->show();
		selection_probability_label->show();
		selection_probability_spin->show();
		selection_probability_label->set_text("Winner Probability");
		break;
	}
}


void
MainWindow::gtk_on_execute()
{
	if (!_problem) {
		Gtk::MessageDialog warning_dialog(*main_win,
			"You must open a problem first!",
			false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		warning_dialog.run();
		return;
	}

	_best.reset();

	const size_t population_size       = population_spin->get_value_as_int();
	const size_t num_elites            = elites_spin->get_value_as_int();
	const float  mutation_probability  = mutation_scale->get_value();
	const float  crossover_probability = crossover_probability_scale->get_value();
	const size_t trial_size            = trial_size_spin->get_value_as_int();
	const float  selection_probability = selection_probability_spin->get_value();

	shared_ptr< Problem<TSP::GeneType> > p = _problem;
	//shared_ptr< Problem<TSP::GeneType> > p(new TSP("../data/berlin52.txt"));
	//shared_ptr< Problem<TSP::GeneType> > p(new TSP("../data/square.txt"));
	
	shared_ptr< Selection<TSP::GeneType> > s;

	switch (selection_combo->get_active_row_number()) {
	case 0:
		s = shared_ptr< Selection<TSP::GeneType> >(
				new RouletteSelection<TSP::GeneType>(p));
		break;
	case 1:
		s = shared_ptr< Selection<TSP::GeneType> >(
				new RankSelection<TSP::GeneType>(p, selection_probability));
		break;
	case 2:
		s = shared_ptr< Selection<TSP::GeneType> >(
				new TournamentSelection<TSP::GeneType>(p, trial_size, selection_probability));
		break;
	}
		
	_ga = shared_ptr<TSPGA>(new TSPGA(p, s, _crossover, _mutation,
			p->gene_size(), population_size, num_elites,
			mutation_probability, crossover_probability));
	
	Glib::Thread::create(
			sigc::mem_fun(this, &MainWindow::ga_run),
			1024, false, true, Glib::THREAD_PRIORITY_NORMAL);

	Glib::signal_timeout().connect(sigc::mem_fun(this, &MainWindow::gtk_iteration), 200);
}


void
MainWindow::gtk_on_stop()
{
	_ga = shared_ptr<TSPGA>();
}


void
MainWindow::gtk_on_open()
{
	Gtk::FileChooserDialog* dialog = new Gtk::FileChooserDialog(
		*main_win, "Load Problem", Gtk::FILE_CHOOSER_ACTION_OPEN);
	
	dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog->add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	int result = dialog->run();
	const string filename = dialog->get_filename();
	delete dialog;

	if (result == Gtk::RESPONSE_OK) {
		_problem = boost::shared_ptr<TSP>(new TSP(filename));
		viewport->remove();
		_canvas = TSPCanvas::load(_problem);
		viewport->add(*_canvas.get());
		_canvas->show();
	}
}


void
MainWindow::gtk_on_about()
{
	about_dialog->run();
	about_dialog->hide();
}
	

bool
MainWindow::gtk_iteration()
{
	static bool first_run = true;

	if (!_ga) {
		first_run = true;
		return false;
	}

	if (_ga->generation() == 0)
		return true;

	assert(_canvas);

	if (first_run) {
		_canvas->zoom_full();
		first_run = false;
	}

	ostringstream gen_ss;
	gen_ss << _ga->generation();
	generation_label->set_text(gen_ss.str());
	
	shared_ptr<TSPGA> tspga = dynamic_pointer_cast<TSPGA>(_ga);
	assert(tspga);

	const shared_ptr<const TSP::GeneType> best = tspga->best();
	assert(best);

	if (best && best != _best) {
		ostringstream fit_ss;
		fit_ss << tspga->problem()->fitness(*best.get());
		shortest_path_label->set_text(fit_ss.str());
		best_generation_label->set_text(gen_ss.str());

		_canvas->update(best);

		_best = best;
	}
	
	return true;
}


} // namespace GUI
} // namespace Eugene
