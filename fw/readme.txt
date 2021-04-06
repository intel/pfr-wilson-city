##################
# Build
##################
Before building in this directory, execute the followings:
	% cd ../impl/wilson_city
	% make prep
	% cd -
That target generates the Qsys system files as well as some header files. 

After the steps above, simply run: 
	% make
The default target creates the BSP first, then builds the Nios code and runs unittests against its source files. 

##################
# Development
##################
You can bring up eclipse with: 
	% make eclipse
In Eclipse GUI, you can import the projects by: 
	File => import... => Existing Projects into Workspace
	Select "select root directory" and click "Browse"
	Open the entire "fw" directory
	Select all (pfr_sys_bsp, pfr_sys_src, unit_test) and Finish
Also, you need to change some settings:
1. Use spaces instead of tab: 
	Window => Preferences => C/C++ => Code Style => Formatter => New...
	Give it a name and change Tab Policy to use spaces
2. Set up build command
	In project explorer, right click on "pfr_sys_src" and select "properties". 
	Go to "C/C++ Build" and uncheck "Use default build command"
	Copy the fw build command from prep_revision.sh (e.g. make -C fw/code PLATFORM_NAME=wilson_city PLATFORM_HW=pfr_po all)

Format source code with:
	% clang-format -i <file>

##################
# Unittests
##################
We use Google Test framework to perform unittests on Nios code. 
You can execute these tests by:
	% cd test
	% make

##################
# Code Coverage
##################
GCov/Lcov is enabled when performing unit-testing with Google test. 
You can run these unittests:
	% cd test
	% make test-coverage
You can view the result with a web browser. Example:
	% firefox coverage_result

##################
# Coverity
##################
You can perform coverity check (fail if there's any violation) with: 
	% make coverity-check

You can perform coverity scan and submit the results to PSG server with: 
	% make coverity-scan
Currently, it will ask you for password to authenticate your account on the server. 
You can download a key file from your account on the web app of the server (http://sj-coverity.altera.com:5467). Then you can automate this step by supply that key file (COV_KEY_FILE=<your file>) to the command. 


