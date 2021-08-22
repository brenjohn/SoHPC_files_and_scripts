
using MPI

include("min_width_sampling.jl")

MPI.Init()
comm = MPI.COMM_WORLD
my_rank = MPI.Comm_rank(comm)
root = 0

print("rank $my_rank has $(Threads.nthreads()) threads \n")



#=
    Each rank should load in graph and use min width to find an 
    elimination order.
=#

g = graph_from_gr("sycamore_53_8_0.gr")
order, tw = min_width_mt_sampling(g, 100, 42+my_rank)



#=
    Find which rank has the best treewidth.
=#

"""returns the better (rank, treewidth) pair"""
function best(order1::Tuple{Int, Int}, order2::Tuple{Int, Int})
    order1[2] < order2[2] ? order1 : order2
end

rank_with_best, best_tw = MPI.Allreduce((my_rank, tw), best, comm)
MPI.Barrier(comm)



#=
    get the rank with the best treewidth to send its elimination
    order to the root rank.
=#

if my_rank == root
    best_order = similar(order)
    rreq = MPI.Irecv!(best_order, rank_with_best, rank_with_best, comm)
end

if my_rank == rank_with_best
    print("rank $my_rank sending elimination order to root \n")
    sreq = MPI.Isend(order, root, my_rank, comm)
end

MPI.Barrier(comm)

if MPI.Comm_rank(comm) == root
    @show best_tw
    @show best_order
end