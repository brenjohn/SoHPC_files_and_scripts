### A Pluto.jl notebook ###
# v0.15.0

using Markdown
using InteractiveUtils

# ╔═╡ a7cf6339-4de6-4a11-bdaf-fe89aad4aa30
begin
    import Pkg
    # activate a temporary environment
    Pkg.activate(mktempdir())
    Pkg.add([
        Pkg.PackageSpec(name="GraphRecipes", version="0.5.2"),
		Pkg.PackageSpec(name="LightGraphs", version="1.3.5"),
		Pkg.PackageSpec(name="Plots", version="1.0.14"),
		Pkg.PackageSpec(name="LightGraphsFlows", version="0.4.2"),
		Pkg.PackageSpec(name="DataStructures", version="0.18.9")			
    ])
    using GraphRecipes
	using LightGraphs
	using Plots
	using SparseArrays
	using LightGraphsFlows
	using DataStructures
end

# ╔═╡ 2a633e72-db12-11eb-1f3e-5b326265edd8
md"""# Attempt at implementing graph dissection algorithm
Given a graph G and source and target nodes s and t, we wish to find a dissection which trades off balance and cut size.
"""

# ╔═╡ 7a66ba1c-a340-4f2f-8aaa-4fbe5a995ed8
md"""We Create a simple graph and assign source and target nodes"""

# ╔═╡ 3cc40680-c086-48db-aac2-21633ee3dc34
begin
	g = SimpleGraph(5)
	add_edge!(g, 1, 2)
	add_edge!(g, 1, 3)
	add_edge!(g, 2, 4)
	add_edge!(g, 3, 4)
	add_edge!(g, 4, 5)
	add_edge!(g, 3, 5)
	g = DiGraph(g)
	n = nv(g)
end

# ╔═╡ 10255d75-1afc-46a0-960f-5b1db3c4bd9f
function growset(g, flow_matrix, capacity_matrix, starting_set; reverse=false)
	n = nv(g)
	full_set = BitArray(fill(0, n))
	q = Queue{Int64}()
	enqueue!.([q], findall(x -> x > 0, starting_set))
	if reverse
		flow_matrix = -1 .* flow_matrix
	end
	while length(q) > 0
		a = dequeue!(q)
		full_set[a] = true
		for b in neighbors(g, a)
			if !full_set[b] && flow_matrix[a,b] < capacity_matrix[a,b]
				full_set[b] = true
				enqueue!(q, b)
			end
		end
	end
	full_set
end

# ╔═╡ aaee73af-46d7-4aed-9c76-13ef800329f4
"""
	augment_flow(g, flow_matrix, capacity_matrix, source, target)

Augment the flow by applying one augmenting path. Taken from
https://github.com/JuliaGraphs/LightGraphsFlows.jl/blob/master/src/edmonds_karp.jl
"""
function augment_flow(g, flow_matrix, capacity_matrix, source, target)
	residual_graph = LightGraphsFlows.residual(g)
	v, P, S, flag = LightGraphsFlows.fetch_path(residual_graph,	
												source, 
												target,
												flow_matrix, 
							   					capacity_matrix)

	if flag != 0                       # no more valid paths
		return 0
	else
		path = [Int(v)]                     # initialize path
		sizehint!(path, n)

		u = v
		while u != source                # trace path from v to source
			u = P[u]
			push!(path, u)
		end
		reverse!(path)

		u = v                          # trace path from v to target
		while u != target
			u = S[u]
			push!(path, Int(u))
		end
		# augment flow along path
		return LightGraphsFlows.augment_path!(path, flow_matrix, capacity_matrix)
	end
end

# ╔═╡ 93087f68-f126-4883-a919-15a8fa25cd4d
begin
	fm = zeros(Int, nv(g), nv(g))
	cm = adjacency_matrix(g)
	@show augment_flow(g, fm, cm, 1, 5)
	@show augment_flow(g, fm, cm, 1, 5)
	#@show augment_flow(g, fm, cm, s, t)
	c′ = x -> x == 1 ? colorant"green" : (x == 5 ? colorant"red" : colorant"blue")
	mycolors = c′.(vertices(g))
	graphplot(g, method=:graph, names=vertices(g), nodecolor=mycolors,				
			  edgelabel=map(x -> "$x", fm))
end

# ╔═╡ ee28812b-ed81-4b7b-aed3-5ec447b41711
function find_piercing_node(g, flow_matrix, capacity_matrix, S, T, SR, TR; 
							source_side=true)
	@assert sum(S .| T) < nv(g) "No nodes left that is not a source or a sink"
	x = 1
	while (S[x] > 0) || (T[x] > 0)
		x += 1
	end
	x
end

# ╔═╡ 6a532967-478d-47ec-8286-d49e8d8cae38
function flow_dissection(g, s, t)
	g = copy(g)
	println()
	@show (s, t)
	# add super source and super target
	add_vertex!(g)
	add_vertex!(g)
	super_s = nv(g) - 1
	super_t = nv(g)
	add_edge!(g, super_s, s)
	add_edge!(g, t, super_t)
	
	n = nv(g)
	# create initial flow matrix
	flow_matrix = zeros(n, n)
	
	# create capacity matrix
	capacity_matrix = SparseMatrixCSC{AbstractFloat, Int64}(adjacency_matrix(g))
	# add infinite capacity from and to super source and super target nodes
	capacity_matrix[super_s, s] = Inf
	capacity_matrix[t, super_t] = Inf
	
	S = BitArray(fill(0, n)); S[[super_s, s]] .= 1
	T = BitArray(fill(0, n)); T[[super_t, t]] .= 1
	
	SR = growset(g, flow_matrix, capacity_matrix, S)
	TR = growset(g, flow_matrix, capacity_matrix, T, reverse=true)
	
	while !any(S .& T)
		if sum(S .| T) >= n break end
		@show (S, T), sum(S .| T) 
		@show (SR, TR), SR .& TR
		if any(SR .& TR)
			@show augment_flow(g, flow_matrix, capacity_matrix, super_s, super_t)
			SR = growset(g, flow_matrix, capacity_matrix, S)
			TR = growset(g, flow_matrix, capacity_matrix, T, reverse=true)
		else
			if sum(SR) <= sum(TR)
				S = growset(g, flow_matrix, capacity_matrix, S)
				#output source side cut edges
				# find piercing node, just pick random one not in S
				x = find_piercing_node(g, flow_matrix, capacity_matrix, S, T, SR, TR; 
									   source_side=true)
				println("grow source $x")
				S[x] = 1
				add_edge!(g, super_s, x)
				capacity_matrix[super_s, x] = Inf
				SR[x] = 1
				SR = growset(g, flow_matrix, capacity_matrix, S)
			else
				T = growset(g, flow_matrix, capacity_matrix, T, reverse=true)
				# output source side cut edges
				# find piercing node, just pick random one not in S
				x = find_piercing_node(g, flow_matrix, capacity_matrix, S, T, SR, TR; 
									   source_side=false)
				println("grow target $x")
				T[x] = 1
				add_edge!(g, x, super_t)
				capacity_matrix[x, super_t] = Inf
				TR[x] = 1
				TR = growset(g, flow_matrix, capacity_matrix, T, reverse=true)
			end
		end
	end
	S, T, flow_matrix, capacity_matrix, g
end

# ╔═╡ 94b6224e-7b11-497d-a625-9535051e6500
s′, t′, fm′, cm′, g′ =  flow_dissection(g, 1, 4)

# ╔═╡ 186d5659-480d-4ab1-8027-280c69da7ab3
begin
	color_sides = x -> s′[x] > 0 ? colorant"red" : colorant"blue"
	graphplot(g′, method=:graph, names=vertices(g′),				
			  edgelabel=map(x -> "$x", fm′),
			  nodecolor=color_sides.(vertices(g′)))
	
end

# ╔═╡ Cell order:
# ╟─2a633e72-db12-11eb-1f3e-5b326265edd8
# ╠═a7cf6339-4de6-4a11-bdaf-fe89aad4aa30
# ╟─7a66ba1c-a340-4f2f-8aaa-4fbe5a995ed8
# ╠═3cc40680-c086-48db-aac2-21633ee3dc34
# ╠═10255d75-1afc-46a0-960f-5b1db3c4bd9f
# ╠═aaee73af-46d7-4aed-9c76-13ef800329f4
# ╠═93087f68-f126-4883-a919-15a8fa25cd4d
# ╠═ee28812b-ed81-4b7b-aed3-5ec447b41711
# ╠═6a532967-478d-47ec-8286-d49e8d8cae38
# ╠═94b6224e-7b11-497d-a625-9535051e6500
# ╠═186d5659-480d-4ab1-8027-280c69da7ab3
