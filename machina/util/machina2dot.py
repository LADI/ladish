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
	rankdir=TD;
	size="20,20"
	node [shape = doublecircle, width = 1.25 ];
""",

node_durations = { }

initial_nodes_query = RDF.SPARQLQuery("""
PREFIX machina: <http://drobilla.net/ns/machina#>
SELECT DISTINCT ?n ?dur WHERE {
	?m  machina:initialNode ?n .
	?n  a                 machina:Node ;
		machina:duration  ?dur .
}
""")

for result in initial_nodes_query.execute(model):
	node_id  = result['n'].blank_identifier
	duration = float(result['dur'].literal_value['string'])
	node_durations[node_id] = duration
	print '\t', node_id, "[ label = \"d:", duration, "\"];"


print "\tnode [shape = circle, width = 1.25 ];"


nodes_query = RDF.SPARQLQuery("""
PREFIX machina: <http://drobilla.net/ns/machina#>
SELECT DISTINCT ?n ?dur WHERE {
	?m  machina:node      ?n .
	?n  a                 machina:Node ;
		machina:duration  ?dur .
}
""")

for result in nodes_query.execute(model):
	node_id  = result['n'].blank_identifier
	duration = float(result['dur'].literal_value['string'])
	node_durations[node_id] = duration
	print '\t', node_id, "[ label = \"d:", duration, "\"]; "

	
edge_query = RDF.SPARQLQuery("""
PREFIX machina: <http://drobilla.net/ns/machina#>
SELECT DISTINCT ?tail ?head ?prob WHERE {
	?e  a                   machina:Edge ;
		machina:tail        ?tail ;
		machina:head        ?head ;
		machina:probability ?prob .
}
""")
	
for edge in edge_query.execute(model):
	print '\t', edge['tail'].blank_identifier, ' -> ',
	print edge['head'].blank_identifier, ' ',
	print "[ label = \"", edge['prob'].literal_value['string'], "\" ",
	print "minlen = ", node_durations[edge['tail'].blank_identifier] * 7.5, " ];"

print "}"
