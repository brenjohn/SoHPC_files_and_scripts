# SoHPC_files_and_scripts

Some useful files and scripts for the fast parallel tree decomposition projects.

I generated several variations of the graphs for different circuits of interest as it may be interesting to compare these in a couple of weeks. Initially, however we can just stick to graphs in `circuit_graphs/qflex_line_graph_files_decomposed_true_hyper_true`.

Each graph comes from a different circuit. The graphs named `bristlecone` and `sycamore` come from circuits ran on google's bristlecone and sycamore quantum processors for example. These can get quite big, so maybe the smaller versions are best to start testing with: `test.gr`, `rectangular_2x2_1-16-1_0.gr`, `rectangular_4x4_1-16-1_0.gr`, `sycamore_53_4_0.gr`, `bristlecone_48_1-16-1_0.gr`.

## GR format used
The first line in the `.gr` files contain the number of vertices and edges in the graph. Each subsequent line has two integers (source and destination vertices) describing an edge of the graph

You can use the code from the script `plot_graph_from_gr_file.jl` to read the `.gr` files and create a graph.

## Installing julia on Kay

From your home directory, download julia 1.6.2 and extract from the tar file

```
wget https://julialang-s3.julialang.org/bin/linux/x64/1.6/julia-1.6.2-linux-x86_64.tar.gz
tar -xf julia-1.6.2-linux-x86_64.tar.gz
```

Then create a symbolic link to the extracted binary
```
ln -s ./julia-1.6.2/bin/julia .local/bin/julia
```
