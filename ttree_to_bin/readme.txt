# author: jacob pierce
# date: 9.17

Usage: run_ttree_to_bin <root tree file with extension.root> <-0>

-0 disables the PROTECT mode, which is enabled by default. When enabled, overwriting of an existing directory is forbidden.

ttree_to_bin.cpp is a ROOT script which takes a ROOT tree with a specific format (which would need to be changed in source code in order to use this for something else) and writes data from the tree to a binary file that can be processed in any programming language. 

run_ttree_to_bin is a bash script which makes using the program slightly safer.
