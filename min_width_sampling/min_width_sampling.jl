using LightGraphs
using Random

###
### Utility functions
###

"""
    graph_from_gr(filename::String)

Read a graph from the provided gr file.
"""
function graph_from_gr(filename::String)
    lines = readlines(filename)

    # Create a Graph with the correct number of vertices.
    num_vertices = parse.(Int, split(lines[1], ' ')[3])
    G = SimpleGraph(num_vertices)

    # Add an edge to the graph for every other line in the file.
    for line in lines[2:end]
        src, dst = parse.(Int, split(line, ' '))
        add_edge!(G, src, dst)
    end

    G
end

"""
    eliminate!(graph, labels, vertex)

Connects the neighbours of the given vertex into a clique before removing 
it from the graph.

The array of vertex labels are also updated to reflect the 
reording of vertex indices when the graph is updated.
"""
function eliminate!(g::AbstractGraph, labels, v)
    Nᵥ = all_neighbors(g, v)::Array{Int64, 1}
    for i = 1:length(Nᵥ)-1
        vi = Nᵥ[i]
        for j = i+1:length(Nᵥ)
            vj = Nᵥ[j]
            add_edge!(g, vi, vj)
        end
    end

    rem_vertex!(g, v)
    labels[v] = labels[end]
    pop!(labels)
    g
end

###
### Min Width functions
###

"""
    min_width(g::AbstractGraph, seed=nothing)

Builds a vertex elimination order and corresponding treewidth for `g` using
the min width heuristic and the given `seed` for the random number generater.
"""
function min_width(g::AbstractGraph, seed=nothing)
    labels = collect(1:nv(g))
    order = Array{Int, 1}(undef, nv(g))
    rng = seed isa Int ? MersenneTwister(seed) : MersenneTwister()
    tw = min_width(rng, g, labels, order)
    order, tw
end

"""
    min_width(rng::AbstractRNG, 
              g::AbstractGraph, 
              labels::Array{Int, 1}, 
              order::Array{Int, 1})

Returns a vertex elimination order and corresponding treewidth for `g`.

The vertex elimination order is placed in the given `order` array.
"""
function min_width(rng::AbstractRNG, 
                   g::AbstractGraph, 
                   labels::Array{Int, 1}, 
                   order::Array{Int, 1})
    g = deepcopy(g)
    tw = 0

    i = 1
    n = nv(g)
    while i <= n
        d_map = degree(g)
        candidates = findall(isequal(minimum(d_map)), d_map)
        v = rand(rng, candidates)
        tw = max(tw, degree(g, v))
        order[i] = labels[v]
        eliminate!(g, labels, v)
        i += 1
    end

    tw
end

"""
    min_width_sampling(g::AbstractGraph, N_samples::Int)

Use the min width heuristic `N_samples` times and return the best result.
"""
function min_width_sampling(g::AbstractGraph, N_samples::Int)
    rng = MersenneTwister()

    best_order = Array{Int, 1}(undef, nv(g))
    best_tw = nv(g)
    
    curr_order = Array{Int, 1}(undef, nv(g))

    for i = 1:N_samples
        curr_tw = min_width(rng, deepcopy(g), collect(1:nv(g)), curr_order)
        if curr_tw < best_tw
            best_tw = curr_tw
            best_order[:] = curr_order[:]
        end
    end

    best_order, best_tw
end

function min_width_mt_sampling(g::AbstractGraph, samples_per_thread::Int)
    orders = Array{Array{Int, 1}, 1}(undef, Threads.nthreads())
    tws = Array{Int, 1}(undef, Threads.nthreads())

    Threads.@threads for _ = 1:Threads.nthreads()
        orders[Threads.threadid()], tws[Threads.threadid()] = min_width_sampling(g, samples_per_thread)
    end

    best = argmin(tws)
    orders[best], tws[best]
end

###
### Functions for checking the treewidth of an elimination order
###

"""
    find_treewidth_from_order(G::AbstractGraph, order::Array{Symbol, 1})

Return the treewidth of `G` with respect to the elimination order in `order`.
"""
function find_treewidth_from_order(G::AbstractGraph, order::Array{Int, 1})
    G = deepcopy(G)
    labels = collect(1:nv(G))
    τ = 0
    for v_label in order
        v = findfirst(vl -> vl == v_label, labels)
        τ = max(τ, degree(G, v))
        eliminate!(G, labels, v)
    end
    τ
end

"""Eliminate vertex v from G and update the labels array accordingly"""
function eliminate!(G, labels, v)
    N = all_neighbors(G, v)
    for i = 1:length(N)-1
        for j = i+1:length(N)
            add_edge!(G, i, j)
        end
    end

    labels[v] = labels[end]
    rem_vertex!(G, v)
end

###
### Example
###

g = graph_from_gr("sycamore_53_8_0.gr")
order, tw = min_width(g)
order, tw = min_width_sampling(g, 100)
order, tw = min_width_mt_sampling(g, 25)

println(tw == find_treewidth_from_order(g, order))