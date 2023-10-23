# ValueCheck

This repo is for code release of our paper `Effective Bug Detection with Unused Definitions` in `EuroSys 2024`.   

In the paper we propose to use cross-author unused definitions to detect bugs and prioritize bugs by its familarity.  

Its workflow contains three steps:
* With the bitcode as input, ValueCheck applies static analysis to identify unused definitions and prune false positives.
* From the found snippets, ValueCheck extracts authorship information and capture cross-authorship relationship.
* After calculating the code familiarity, ValueCheck prioritizes the detected unused definitions.

This repo contains the source code, scrips, and other artifacts. These are required to reproduce the results we presented in the paper.
It helps the reproduction of evaluation results in the section 8 of the paper. 

The artifact is available on GitHub at https://github.com/floridsleeves/ValueCheck.

## Software dependencies
- Linux (tested on Ubuntu 20.04)
- Python >=3.8
- SVF >= 2.7
- LLVM >= 12.0
- git-lfs >= 2.9.2

### git-lfs
In the `run.sh` script we include the automatically set up for git-lfs. However, your installation might slightly change according to the [instructions](https://docs.github.com/en/repositories/working-with-files/managing-large-files/installing-git-large-file-storage?platform=linux).

You can use `git lfs clone` to download this repo to your local machine. In some new version of gits, `git clone` has been updated in upstream Git to have comparable
speeds to `git lfs clone`.
  
## Data sets
- The artifact evaluates four open-source web applications. The scripts will automatically download their source code from GitHub and checkout the corresponding versions. 
- The directory `bitcode` in the artifact includes the pre-compiled bitcode from each application by `wllvm` with flag `-fno-inline` `-O0` and `-g`. The bitcodes are broken into different modules to reduce the inter-procedural value analysis time of SVF.

## Steps to reproduce 
- We provide a script `./install.sh` to automatically install the dependencies and build the software. 
- We provide a script `./run.sh` to automatically perform the evaluation. 

```python
./install.sh
# Step 0: Clean the previous output files
# Step 1: Install dependencies, compile SVF
# Step 2: Compile ValueCheck

./run.sh
# Step 3: Run ValueCheck - the analysis tool. 
# Step 4: Run ValueCheck and produce the result
```

## Evaluation and expected results
We provide the scripts to automate the evaluation and generate the Tables and numbers in Section 4. 
The output will be in the `result/` folder and contain the following key results:
- `result/table_2_detected_bugs.csv`: 
  - Total number of detected bugs from each application. (`Table 2`) 
- `result/table_6_dok_effect.csv`: 
  - The number of detected bugs within top 20 bugs under different DOK settings. (`Table 6`)
- `result/figure_7_dist.pdf`, `result/figure_7_security.pdf`, `result/figure_7_days.pdf`: 
  - The category of bugs based on distribution, security, and days before detected. (`Figure 7`)
- `result/figure_9_detected_bug_dok.pdf`: 
  - The figure of reported bugs when increasing DOK rank. (`Figure 9`)
- `result/table_7_time_analysis.csv`: 
  - Time (seconds) to run the analysis. (First column of `Table 7`)

- In the `result/APP_NAME/` directory, `detected.csv` contains all the detected bugs.

Note that some results involve random sampling (second and third columns in `Table 6`) and developers' confirmation (last column in `Table 2`), thus not included in the artifact. 
Note that due to the differences in hardware environments and the optimization we later add to the tool, the performance results in `Table 7` can be different from the numbers reported in the paper.
