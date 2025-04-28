# MacroSim

These SST components comprise of a simulator used to implement and validate the MacroSim methodology. The simulator uses input Workload Description Distributions (WDD) and generates performance projections.

## Instructions
Follow the instructions [here](https://sst-simulator.org/SSTPages/SSTBuildAndInstall_11dot0dot0_SeriesDetailedBuildInstructions/) to install SST-core.

MacroSim has been tested on SST-core 13.0.0.

### Building libtdl for use with autotools
```
mkdir -p $HOME/local
cd $HOME/scratch/src
wget http://ftp.gnu.org/gnu/libtool/libtool-2.4.6.tar.gz
tar xvzf libtool-2.4.6.tar.gz
./configure -C --prefix=$HOME/local
export PATH=$HOME/local/bin:$PATH
```
Please note that the libtldl version used for MacroSim is available in src/libtldl

### Installing MacroSim
Installing MacroSim is done as follows:
``` bash
$ export SST_CORE_HOME=/your/sst/core
$ export PATH=$SST_CORE_HOME/bin:$PATH
$ git clone https://github.com/prachatos/macrosim.git
$ git checkout main
$ export MACROSIM_HOME=/your/macrosim/path
$ cd /your/macrosim/path
$ ./autogen.sh
$ ./configure --with-sst-core=$SST_CORE_HOME --prefix=$MACROSIM_HOME
$ make all
$ make install
```

## Folder structure
The ``src`` folder contains the source code for MacroSim and the ``apps`` folder contains the WDDs for all workloads.

The ``test-scripts`` folder contains the following files:
  - `macrosim-FatTree-ZAP.py` - Base script for any experiments (-h has an exhaustive list of commands)
  - `run_tc.sh` - Runs all TC experiments
  - `run_bfs.sh` - Runs all BFS experiments
  - `run_jaccard.sh` - Runs all Jaccard experiments
  - `run_aggr_exp.py` - Runs aggregation experiments

The ``utils`` folder contains the following files:
  - `collect_aggr_data.py` - Compiles the data from the aggregation experiment to a CSV file summarizing the metrics
  - `plot_aggr_graph.py` - Uses the CSV output from the aggregation experiment post-processing script and plots the figure (Figure 10)
  - `get_proc_msg.py` - Measures number of messages processed for a workload (metric for Figures 7 and 8)

## Testing

To execute a simulation, please run:

``` bash
sst --stop-at=8ms --print-timing-info=true test-scripts/macrosim-FatTree-ZAP.py -- --shape="1,1:2" --pes=8 --zaps=4 --zones=8 --app=app_name --enableStatistics 1 --demo_mode=0 --event_driven_send=1 --event_driven_recv=1 --read_slots=520 --clock_speed=2GHz --max_aggr_size=0 --stat_path="stat_path/app_name.csv"
```

For instance, to run TC with the scaled distribution for 512 PEs, run:
``` bash
sst --stop-at=8ms --print-timing-info=true test-scripts/macrosim-FatTree-ZAP.py -- --shape="1,1:2" --pes=8 --zaps=4 --zones=8 --app=final/tc/scaled/512 --enableStatistics 1 --demo_mode=0 --event_driven_send=1 --event_driven_recv=1 --read_slots=520 --clock_speed=2GHz --max_aggr_size=0 --stat_path="tc_scaled_512.csv"
```

Note that a PE in this description corresponds to a pair of send and receive threads. Therefore, the clock speed is set to 2GHz (total 4GHz). The read slots are adjusted to fit our target bandwidth requirement.
All other parameters use the defaults in the script as mentioned in the paper.

Once an experiment is executed, please use ``utils/get_proc_msg.py`` to measure the number of messages processed in the given time. The ratio of these is the message rate.
The command is as follows:
`` bash
python3 utils/get_proc_msg.py mpi_ranks stat_path extension
``

For instance:
`` bash
python3 utils/get_proc_msg.py 1 tc_scaled_512 csv
``
