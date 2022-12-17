

# Privateer
![ScreenShot](/logo.png)
**Synopsis:** Privateer is a tool for carbohydrate structure validation, re-refinement and graphical analysis.

**Authors:** Jon Agirre (@glycojones), with contributions from Haroldas Bagdonas (@GABRAH), Jordan Dialpuri (@Dialpuri) and Lucy Schofield (@lcs551) 

**Languages:** C++11 and Python3 (via pybind11), produces Scheme and Python scripts for use with Coot (https://github.com/pemsley/coot).

**Dependencies:** Gemmi (https://github.com/project-gemmi/gemmi), Pybind11 (https://github.com/pybind/pybind11), CMake and a few CCP4 LGPLv3-compatible development libraries.


## **Build it yourself:**
Stable versions of Privateer are routinely released by CCP4 and CCP-EM. While we recommend that users download and install those suites for X-ray crystallography and Electron cryo-microscopy respectively, we recognise that they do not always come with the latest functionality. Is you would like to test the latest versions, you'll need to build Privateer yourself. It is not as hard as it sounds, thanks in part to the work Haroldas Bagdonas (@GABRAH) put into a build script that gathers and builds all dependencies.  

**Operating systems supported** - **MacOS** (tested up to Ventura) and **GNU/Linux** (tested on Ubuntu 22.04). **MacOS** might require additional pre-installation steps (scroll down).

**Other software you'll need to install in order to build Privateer:** 

**bzr** (breezy on Mac homebrew)
> This is CCP4's version control system, and it will be required until CCP4 starts using git.

**cmake** (minimum version required 3.12)
> Privateer itself requires cmake.

**python3 virtualenv coreutils wget** 
> Python3 and virtualenv will allow you to have you own little Python with access to the Privateer module (import privateer). We need coreutils and wget so the build scripts can function.

**gcc g++ gfortran m4** (Debian/Ubuntu: apt-get install buildessential m4 gfortran; Mac OS X: brew install gcc gfortran m4)
> These are the compilers, for C/C++/Fortran

0. Select your preferred Python (up to 3.10 suppported) using pyenv install [version] and then pyenv global [version]

1. git clone https://github.com/glycojones/privateer.git
> Creates a copy of the repository on your machine and switches it to the *master* branch.

2. cd privateer
> Goes into the directory with the repository, which has been created by the previous command.

3. git submodule update --init --recursive
> This will fetch those dependencies that are managed by git.

4. virtualenv privateerpython
> It creates your own Python environment within the repository.

5. source ccp4.envsetup-sh
> CCP4 environment variables, required for Privateer to work.

6. source privateerpython/bin/activate
> Will setup the Python environment.

7. pip install -r requirements.txt
> Installs the list of Python dependencies.

8. python setup.py install
> This is the one command that actually builds something. It will take a while.



## An example of validating a model:

**C++** executable can be accessed in: **build/executable/./privateer** directory

Before executing Privateer it is important to have ccp4.envsetup-sh properly sourced!

1.) cd build/executable

2.) mkdir tmp

3.) cd tmp

4.) .././privateer -pdbin ../../../tests/test_data/5fjj.pdb -mtzin ../../../tests/test_data/5fjj.mtz -glytoucan -databasein ../../../src/privateer/privateer_database.json -debug_output -all_permutations

Privateer's Python bindings(assuming that the virtualenv is not deactivated) can also be accessed via: 

1.) python

2.) from privateer import privateer_core as pvt

3.) print(dir(pvt))

4.) 

    wurcs = pvt.print_wurcs("/Users/haroldas/Dev/privateer_dev_noccp4/tests/test_data/2h6o_carbremediation.pdb")
    
    print(wurcs)


## PRE-INSTALLATION INSTRUCTIONS FOR macOS CATALINA: 

**Requirements:**

**Homebrew** and write permissions given to it via sudo. 

1.) Download and install Command_Line_Tools_for_Xcode_12.dmg from https://developer.apple.com/download/more/

2.) brew install gcc@9 (Versions > gcc@9 don't seem to work, as gcc@10 gives compilation error for Fortran dependencies)

3.) brew cask install gfortran

4.) 

    ln -sf /usr/local/bin/gcc-9 /usr/local/bin/gcc
    
    ln -sf /usr/local/bin/gcc-9 /usr/local/bin/cc
    
    ln -sf /usr/local/bin/g++-9 /usr/local/bin/g++
    
    ln -sf /usr/local/bin/c++-9 /usr/local/bin/c++

Ensure that Homebrew's /usr/local/bin comes before /usr/bin in **echo $PATH** command!

For example: /Users/haroldas/anaconda3/condabin:/Users/haroldas/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin

Useful troubleshoot threads in case overkill methods become a necessity: 

https://apple.stackexchange.com/questions/245891/installed-gcc-with-homebrew-now-how-to-use-that-gcc-instead-of-clang

https://apple.stackexchange.com/questions/99077/how-to-set-gcc-4-8-as-default-gcc-compiler/99157#99157

After pre-installation steps are complete, one can carry on as normal in terms of installation instructions. 


*If there are any issues with the installation process or running Privateer, please get in touch with: hb1115@york.ac.uk*
_____

Features: Automatic assignment of ring conformation (IUPAC nomenclature), anomeric form, absolute configuration and comparison to reference values for validation. It computes omit mFo-DFc maps and calculates a correlation coefficient between model and electron density. For structure refinement, it is able to generate chemical dictionaries with unimodal torsion restraints which will help keep the lowest energy conformation. In terms of graphical analysis, it will produce vector diagrams in SNFG nomenclature (SVG format), which are annotated using the validation information (ring conformation, anomeric form, etc).

Releases: with CCP4 (www.ccp4.ac.uk) and CCP-EM (coming soon).

If you find Privateer useful, please cite:

- Agirre, J., Iglesias-Fernández, J., Rovira, C., Davies, G.J., Wilson, K.S. and Cowtan, K.D., 2015. Privateer: software for the conformational validation of carbohydrate structures. Nature Structural and Molecular Biology, 22(11), p.833.

- Bagdonas, H., Ungar, D. and Agirre, J., 2020. Leveraging glycomics data in glycoprotein 3D structure validation with Privateer. Beilstein Journal of Organic Chemistry, 16(1), 2523-2533.

The following related articles might be of interest too:

- Agirre, J., Davies, G., Wilson, K. and Cowtan, K., 2015. Carbohydrate anomalies in the PDB. Nature chemical biology, 11(5), p.303.
- Agirre, J., 2017. Strategies for carbohydrate model building, refinement and validation. Acta Crystallographica Section D, 73(2), pp.171-186.
- Agirre, J., Davies, G.J., Wilson, K.S. and Cowtan, K.D., 2017. Carbohydrate structure: the rocky road to automation. Current opinion in structural biology, 44, pp.39-47.
- McNicholas, S. and Agirre, J., 2017. Glycoblocks: a schematic three-dimensional representation for glycans and their interactions. Acta Crystallographica Section D: Structural Biology, 73(2), pp.187-194.
- Hudson, K.L., Bartlett, G.J., Diehl, R.C., Agirre, J., Gallagher, T., Kiessling, L.L. and Woolfson, D.N., 2015. Carbohydrate–aromatic interactions in proteins. Journal of the American Chemical Society, 137(48), pp.15152-15160.

Some examples where Privateer has been useful (thank _you_ for the citations!):

- Hastie, Kathryn M., et al. "Structural basis for antibody-mediated neutralization of Lassa virus." Science 356.6341 (2017): 923-928.
- Gristick, Harry B., et al. "Natively glycosylated HIV-1 Env structure reveals new mode for antibody recognition of the CD4-binding site." Nature Structural and Molecular Biology 23.10 (2016): 906.
- Walls, Alexandra C., et al. "Glycan shield and epitope masking of a coronavirus spike protein observed by cryo-electron microscopy." Nature Structural and Molecular Biology 23.10 (2016): 899.
- Pallesen, Jesper, et al. "Structures of Ebola virus GP and sGP in complex with therapeutic antibodies." Nature microbiology 1.9 (2016): 16128.
- Han, Ling, et al. "Divergent evolution of vitamin B9 binding underlies Juno-mediated adhesion of mammalian gametes." Current Biology 26.3 (2016): R100-R101.
- Bokhove, Marcel, et al. "A structured interdomain linker directs self-polymerization of human uromodulin." Proceedings of the National Academy of Sciences 113.6 (2016): 1552-1557.
- Wang, Haoqing, et al. "Asymmetric recognition of HIV-1 Envelope trimer by V1V2 loop-targeting antibodies." Elife 6 (2017).
- Raj, Isha, et al. "Structural basis of egg coat-sperm recognition at fertilization." Cell 169.7 (2017): 1315-1326.
- Snijder, Joost, et al. "An Antibody Targeting the Fusion Machinery Neutralizes Dual-Tropic Infection and Defines a Site of Vulnerability on Epstein-Barr Virus." Immunity 48.4 (2018): 799-811.
- Wu, Liang, et al. "Structural characterization of human heparanase reveals insights into substrate recognition." Nature Structural and Molecular Biology 22.12 (2015): 1016.

#### Funding 
* 2013-2015 BBSRC grant awarded to Keith Wilson (York)
* 2015-2017 BBSRC grant awarded to CCP4/Kevin Cowtan (York)
* 2017-     Royal Society University Research Fellowship awarded to Jon Agirre (York)
* 2018-2021 EPSRC DTP studentship allocated to Jon Agirre & awarded to Mihaela Atanasova (York)
* 2019-2023 Royal Society Research Grant awarded to Jon Agirre (York), covering 4-year studentship awarded to Haroldas Bagdonas

#### Acknowledgements
Lucy Schofield, Marcin Wojdyr, Robbie Joosten, Gideon Davies, Wendy Offen, Christian Roth, Paul Emsley, Garib Murshudov, Eleanor Dodson, Jacob Sorensen, Stuart McNicholas... 
