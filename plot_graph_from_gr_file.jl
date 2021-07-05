using LightGraphs
using GraphPlot

#=
A simple script to read a graph from a gr file and plot it.
=#

"""
    graph_from_gr(filename::String)

Read a graph from the provided gr file.
"""
function graph_from_gr(filename::String)
    lines = readlines(filename)

    # Create a Graph with the correct number of vertices.
    num_vertices, num_edges = parse.(Int, split(lines[1], ' ')[3:end])
    G = SimpleGraph(num_vertices)

    # Add an edge to the graph for every other line in the file.
    for line in lines[2:end]
        src, dst = parse.(Int, split(line, ' '))
        add_edge!(G, src, dst)
    end

    G
end

# The file to read the graph from.
graph_file = "circuit_graphs/qflex_line_graph_files_decomposed_true_hyper_true/bristlecone_48_1-40-1_0.gr"

# Create and plot the graph.
g = graph_from_gr(graph_file)
gplot(g)