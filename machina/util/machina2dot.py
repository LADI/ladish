#!/usr/bin/python

import sys
import RDF

model  = RDF.Model()
parser = RDF.Parser(name="guess")

if len(sys.argv) != 2:
	print "Usage: machina2dot FILE"
	sys.exit(-1)

parser.parse_into_model(model, "file:" + sys.argv[1])


print """
digraph finite_state_machine {
	rankdir=LR;
	size="8,5"
	node [shape = doublecircle];
""",


initial_nodes = RDF.SPARQLQuery("""
PREFIX machina: <http://drobilla.net/ns/machina#>
SELECT ?n ?dur WHERE {
	?m  machina:initialNode ?n .
	?n  a                 machina:Node ;
		machina:duration  ?dur .
}
""").execute(model)

for result in initial_nodes:
	print '\t', result['n'].blank_identifier, "[ label = \"",
	print float(result['dur'].literal_value['string']),
	print "\"]; "

print "\tnode [shape = circle];"


nodes = RDF.SPARQLQuery("""
PREFIX machina: <http://drobilla.net/ns/machina#>
SELECT ?n ?dur WHERE {
	?m  machina:node      ?n .
	?n  a                 machina:Node ;
		machina:duration  ?dur .
}
""").execute(model)

for result in nodes:
	print '\t', result['n'].blank_identifier, "[ label = \"",
	print float(result['dur'].literal_value['string']),
	print "\"]; "


edges = RDF.SPARQLQuery("""
PREFIX machina: <http://drobilla.net/ns/machina#>
SELECT ?tail ?head ?prob WHERE {
	?e  a                   machina:Edge ;
	    machina:tail        ?tail ;
		machina:head        ?head ;
		machina:probability ?prob .
}
""").execute(model);

for result in edges:
	print '\t', result['tail'].blank_identifier, ' -> ',
	print result['head'].blank_identifier, ' ',
	print "[ label = \"", result['prob'].literal_value['string'], "\" ];"

print "\n}"
