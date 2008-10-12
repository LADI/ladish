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

#define GENE_PRINTING 1

#include <algorithm>
#include <bitset>
#include <sstream>
#include <cstring>
#include <fstream>
#include <ctime>
#include <eugene/Gene.hpp>
#include <eugene/GA.hpp>
#include <eugene/GAImpl.hpp>
#include <eugene/ESImpl.hpp>
#include <eugene/Crossover.hpp>
#include <eugene/OnePointCrossover.hpp>
#include <eugene/TwoPointCrossover.hpp>
#include <eugene/UniformCrossover.hpp>
#include <eugene/PartiallyMappedCrossover.hpp>
#include <eugene/OrderCrossover.hpp>
#include <eugene/PositionBasedCrossover.hpp>
#include <eugene/Problem.hpp>
#include <eugene/Mutation.hpp>
#include <eugene/RouletteSelection.hpp>
#include <eugene/TournamentSelection.hpp>
#include <eugene/TSP.hpp>
#include <eugene/LABS.hpp>
#include <eugene/Sphere.hpp>
#include <eugene/Mutation.hpp>
#include <eugene/ESMutation.hpp>

using namespace std;
using namespace Eugene;
		

template <typename G>
void
print_children(const std::pair<G,G>& children)
{
	cout << "\tChild 1: ";
	for (typename G::const_iterator i=children.first.begin(); i != children.first.end(); ++i)
		std::cout << *i << ",";

	cout << endl << "\tChild 2: ";
	for (typename G::const_iterator i=children.second.begin(); i != children.second.end(); ++i)
		std::cout << *i << ",";

	cout << endl;
}


bool key_pressed()
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}


int
main(int argc, char** argv)
{
	srand(time(NULL));
	//srand(1);

	enum Mode { ONE_MAX, SIMPLE_MAX, TSP, CROSSOVER, LABS, SPHERE };
	Mode mode = ONE_MAX;

	// Least robust argument handling ever
	if (argc > 2 && !strcmp(argv[1], "--tsp")) {
		mode = TSP;
	} else if ((argc >= 6 && argc <= 9) && !strcmp(argv[1], "--labs")) {
		mode = LABS;
	} else if (argc > 1 && !strcmp(argv[1], "--one-max")) {
		mode = ONE_MAX;
	} else if (argc > 1 && !strcmp(argv[1], "--simple-max")) {
		mode = SIMPLE_MAX;
	} else if (argc > 1 && !strcmp(argv[1], "--crossover")) {
		mode = CROSSOVER;
	} else if (argc > 2 && !strcmp(argv[1], "--sphere")) {
		mode = SPHERE;
	} else {
		cerr << "USAGE: " << argv[0] << " --one-max"      << endl
			 << "       " << argv[0] << " --simple-max"   << endl
			 << "       " << argv[0] << " --tsp FILENAME" << endl
			 << "       " << argv[0] << " --crossover"    << endl
			 << "       " << argv[0] << " --labs N P M [ 1 | 2 | U ] [ G | L ] L [--quiet]"   << endl
			 << "       " << argv[0] << " --sphere P"    << endl;
		
		return -1;
	}
	
	size_t population_size      = 1000;
	float  mutation_probability = 0.1;

	int limit = 0;
	bool limit_generations = true; // otherwise evaluations
	bool quiet = false;

	if (mode == LABS) {
		quiet = (argc > 8);
		
		if (!quiet)
			cout << "Problem size: " << argv[2] << endl;

		population_size = atoi(argv[3]);
		mutation_probability = atof(argv[4]);

		limit = atoi(argv[7]);
		if (argc > 6) {
			if (argv[6][0] == 'L') {
				if (!quiet)
					cout << "Limit: " << limit << " evaluations" << endl;
				limit_generations = false;
			} else if (!quiet) {
					cout << "Limit: " << limit << " generations" << endl;
			}
		}
	} else if (mode == SPHERE) {
		population_size = atoi(argv[2]);
	}

	if (!quiet) {
		cout << "Population Size:      " << population_size << endl;
		cout << "Mutation Probability: " << mutation_probability << endl;
	}

	if (limit == 0 && !quiet)
		cout << "(Press enter to terminate)" << endl;
	
	GA* ga = NULL;

	// 1) a)
	if (mode == ONE_MAX) {
		typedef GAImpl<OneMax::GeneType> OneMaxGA;

		boost::shared_ptr< Problem<OneMax::GeneType> > p(new OneMax(50));
		boost::shared_ptr< Selection<OneMax::GeneType> > s(
				new RouletteSelection<OneMax::GeneType>(p));
		boost::shared_ptr< Crossover<OneMax::GeneType> > c(
				new OnePointCrossover<OneMax::GeneType>());
		boost::shared_ptr< Mutation<SimpleMax::GeneType> > m(
				new RandomMutation<SimpleMax::GeneType>(0, 1));
		ga = new OneMaxGA(p, s, c, m, 50, population_size, 5, mutation_probability, 1.0);
	
	// 1) b)
	} else if (mode == SIMPLE_MAX) {
		typedef GAImpl<SimpleMax::GeneType> SimpleMaxGA;

		boost::shared_ptr< Problem<SimpleMax::GeneType> > p(new SimpleMax());
		boost::shared_ptr< Selection<SimpleMax::GeneType> > s(
				new RouletteSelection<SimpleMax::GeneType>(p));
		boost::shared_ptr< Crossover<SimpleMax::GeneType> > c(
				new OnePointCrossover<SimpleMax::GeneType>());
		boost::shared_ptr< Mutation<SimpleMax::GeneType> > m(
				new RandomMutation<SimpleMax::GeneType>(1, 10));
		ga = new SimpleMaxGA(p, s, c, m, 10, population_size, 5, mutation_probability, 1.0);

	} else if (mode == TSP) {
		typedef GAImpl<TSP::GeneType> TSPGA;
		
		boost::shared_ptr< Problem<Eugene::TSP::GeneType> > p(new Eugene::TSP(argv[2]));
		boost::shared_ptr< Selection<TSP::GeneType> > s(
				new TournamentSelection<TSP::GeneType>(p, 4, 0.8));
		boost::shared_ptr< Crossover<TSP::GeneType> > c(
				//new OrderCrossover<TSP::GeneType>());
				//new PositionBasedCrossover<TSP::GeneType>());
				new PartiallyMappedCrossover<TSP::GeneType>());
		boost::shared_ptr< Mutation<TSP::GeneType> > m(
				new SwapAlleleMutation<TSP::GeneType>());
		ga = new TSPGA(p, s, c, m, p->gene_size(), population_size, 5, mutation_probability, 1.0);
	
	} else if (mode == LABS) {
		typedef GAImpl<LABS::GeneType> LABSGA;

		size_t gene_size = atoi(argv[2]);
		boost::shared_ptr< Problem<Eugene::LABS::GeneType> > p(new Eugene::LABS(gene_size));
		boost::shared_ptr< Selection<LABS::GeneType> > s(
				new TournamentSelection<LABS::GeneType>(p, 3, 0.95));
		boost::shared_ptr< Crossover<LABS::GeneType> > c;
		switch (argv[5][0]) {
		case '1':
			if (!quiet)
				cout << "Using one point crossover" << endl;
			c = boost::shared_ptr< Crossover<LABS::GeneType> >(
					new OnePointCrossover<LABS::GeneType>());
			break;
		case '2':
			if (!quiet)
				cout << "Using two point crossover" << endl;
			c = boost::shared_ptr< Crossover<LABS::GeneType> >(
					new TwoPointCrossover<LABS::GeneType>());
			break;
		case 'U':
			if (!quiet)
				cout << "Using uniform crossover" << endl;
			c = boost::shared_ptr< Crossover<LABS::GeneType> >(
					new UniformCrossover<LABS::GeneType>());
			break;
		default:
			cerr << "Unknown crossover, exiting" << endl;
			return -1;
		}
		boost::shared_ptr< Mutation<LABS::GeneType> > m(
				//new FlipMutation<LABS::GeneType>());
				//new FlipRangeMutation<LABS::GeneType>());
				new FlipRandomMutation<LABS::GeneType>());
		ga = new LABSGA(p, s, c, m, p->gene_size(), population_size, 3, mutation_probability, 0.8);
	
	} else if (mode == SPHERE) {
		typedef ESImpl<Sphere::GeneType> SphereES;

		size_t gene_size = 3;

		boost::shared_ptr< Problem<Sphere::GeneType> > p(new Sphere(gene_size));
		boost::shared_ptr< Selection<Sphere::GeneType> > s(
				new RouletteSelection<Sphere::GeneType>(p));
		boost::shared_ptr< Crossover<Sphere::GeneType> > c; // no crossover (ES)
		boost::shared_ptr< ESMutation<Sphere::GeneType> > m(
				new ESMutation<Sphere::GeneType>());
		ga = new SphereES(p, s, c, m, gene_size, population_size);

	} else if (mode == CROSSOVER) {
		/*string p1_str, p2_str;
		cout << "Enter parent 1: ";
		getline(cin, p1_str);
		cout << endl << "Enter parent 2: ";
		getline(cin, p2_str);
		cout << endl;*/
		
		boost::shared_ptr< Problem<Eugene::TSP::GeneType> > p(new Eugene::TSP());

		string p1_str = "0 2 4 6 8 1 3 5 7 9";
		string p2_str = "9 8 7 6 5 4 3 2 1 0";

		TSP::GeneType p1(0);
		std::istringstream p1_ss(p1_str);
		while (!p1_ss.eof()) {
			size_t val;
			p1_ss >> val;
			p1.push_back(val);
		}
		assert(p->assert_gene(p1));
		
		TSP::GeneType p2(0);
		std::istringstream p2_ss(p2_str);
		while (!p2_ss.eof()) {
			size_t val;
			p2_ss >> val;
			p2.push_back(val);
		}
		assert(p->assert_gene(p2));
		
		std::pair<TSP::GeneType,TSP::GeneType> children
			= make_pair(TSP::GeneType(0), TSP::GeneType(0));

		cout << "Single point:" << endl;
		boost::shared_ptr< Crossover<SimpleMax::GeneType> > c(
				new OnePointCrossover<SimpleMax::GeneType>());
		children = c->crossover(p1, p2);
		print_children(children);
		
		cout << endl << "Partially mapped:" << endl;
		c = boost::shared_ptr< Crossover<SimpleMax::GeneType> >(
				new PartiallyMappedCrossover<SimpleMax::GeneType>());
		children = c->crossover(p1, p2);
		print_children(children);
		assert(p->assert_gene(children.first));
		assert(p->assert_gene(children.second));
		
		cout << endl << "Order based:" << endl;
		c = boost::shared_ptr< Crossover<SimpleMax::GeneType> >(
				new OrderCrossover<SimpleMax::GeneType>());
		children = c->crossover(p1, p2);
		print_children(children);
		assert(p->assert_gene(children.first));
		assert(p->assert_gene(children.second));
		
		cout << endl << "Position based:" << endl;
		c = boost::shared_ptr< Crossover<SimpleMax::GeneType> >(
				new PositionBasedCrossover<SimpleMax::GeneType>());
		children = c->crossover(p1, p2);
		print_children(children);
		assert(p->assert_gene(children.first));
		assert(p->assert_gene(children.second));

		return 0;

	} else {
		cerr << "Unknown mode.  Exiting." << endl;
		return -1;
	}
		
	assert(ga);

	float best_fitness = ga->best_fitness();
	if (!quiet) {
		cout << endl << "Initial best: ";
		ga->print_best();
	}

	while ( ! key_pressed()) {

		ga->iteration();
		float this_best_fitness = ga->best_fitness();
		if (!quiet) {
			if (this_best_fitness != best_fitness) {
				cout << endl << "Generation " << ga->generation() << ": ";
				ga->print_best();
				best_fitness = this_best_fitness;
			}/* else {
				cout << ".";
				cout.flush();
			}*/
		}

		if (ga->optimum_known()) {
			if (best_fitness == ga->optimum()) {
				cout << "Reached optimum" << endl;
				break;
			}
		}

		if (limit > 0) {
			if (limit_generations && ga->generation() >= limit)
				break;
			else if (!limit_generations && ga->evaluations() >= limit)
				break;
		}
	}

	if (!quiet)
		cout << endl << endl;

	cout << "Finished at generation " << ga->generation() << endl;
	cout << "Best individual: ";
	ga->print_best();
	if (ga->optimum_known())
		cout << "(Optimum " << ga->optimum() << ")" << endl;

	ofstream ofs;
	ofs.open("population.txt");
	ga->print_population(ofs);
	ofs.close();

	//cout << endl << "Population written to population.txt" << endl;

	//cout << "Final Elites:" << endl;
	//ga->print_elites();
	
	delete ga;
		
	return 0;
}
