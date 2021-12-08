#ifdef __linux__
    #define SYSTEM_IS_LINUX // uncomment if using on a Linux system
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#elif defined(_WIN32)
    #define SYSTEM_IS_WINDOWS // uncomment if using on a Windows system
    #include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdlib.h>

std::string config_path;

//     IMPORTANT GLOBAL DEFINITIONS
//=============================================================================================================
//=============================================================================================================
//=============================================================================================================

#define COMPILER_IS_TEXLIVE // uncomment if using the TeX live compiler
//#define COMPILER_IS_MIKTEX // uncomment if using the MiKTeX compiler

//#define USE_CONFIG_FILE_DEFAULTS // uncomment if you'd rather use config.txt file to set defaults

/* latest additions:
       > default values now usable for all fields
       > introduced macros for easier cross-platform use
       > added support for setting defaults in config.txt
       > added support for linux
*/
const char *version = "1.5.3";

std::string dont_use_specvalue = "none"; // this value in a specifier-value pair indicates that the default value should not be used

#ifndef USE_CONFIG_FILE_DEFAULTS // don't change this line

bool refresh_viewer = false; // if true, on linux systems the viewer will be manually refreshed using kill -1

// default values for all specifiers, set these only if NOT using config.txt file (see #defines above)
std::string default_master;
std::string default_engine     = "pdflatex";
std::string default_options;
std::string default_bib;
std::string default_biboptions;
std::string default_outext     = ".pdf";
#ifdef SYSTEM_IS_LINUX
std::string default_openwith   = "okular";
std::string default_outoptions;
#elif defined(SYSTEM_IS_WINDOWS)
std::string default_openwith   = "C:\\Program Files\\SumatraPDF\\SumatraPDF.exe";
std::string default_outoptions = "-reuse-instance";
#endif

//=============================================================================================================
//=============================================================================================================
//=============================================================================================================

#else

std::string default_master, default_engine, default_options, default_bib, default_biboptions, default_outext, default_openwith, default_outoptions;

#endif

void read_config_file()
{
    std::ifstream ifile;
    std::string line;

    ifile.open("config.txt");

    std::cout << "\nReading config.txt...\n" << std::endl;

    while(getline(ifile, line))
    {
        if(line.substr(0, 7) == "master=")
        {
            default_master = line.substr(7);
            std::cout << "Default for master set to " << default_master << std::endl;
        }
        else if(line.substr(0, 7) == "engine=")
        {
            default_engine = line.substr(7);
            std::cout << "Default for engine set to " << default_engine << std::endl;
        }
        else if(line.substr(0, 8) == "options=")
        {
            default_options = line.substr(8);
            std::cout << "Default for options set to " << default_options << std::endl;
        }
        else if(line.substr(0, 4) == "bib=")
        {
            default_bib = line.substr(4);
            std::cout << "Default for bib set to " << default_bib << std::endl;
        }
        else if(line.substr(0, 11) == "biboptions=")
        {
            default_biboptions = line.substr(11);
            std::cout << "Default for biboptions set to " << default_biboptions << std::endl;
        }
        else if(line.substr(0, 7) == "outext=")
        {
            default_outext = line.substr(7);
            std::cout << "Default for outext set to " << default_outext << std::endl;
        }
        else if(line.substr(0, 9) == "openwith=")
        {
            default_openwith = line.substr(9);
            std::cout << "Default for openwith set to " << default_openwith << std::endl;
        }
        else if(line.substr(0, 11) == "outoptions=")
        {
            default_outoptions = line.substr(11);
            std::cout << "Default for outoptions set to " << default_outoptions << std::endl;
        }
        else
        {
            std::cout << "I don't know what '" << line << "' means" << std::endl;
        }
    }
    std::cout << std::endl;

    ifile.close();
}

void sanitise_path(std::string &path)
{
    /*
    Both CreateProcess for windows and Unix-like systems prefer a forward slash (/) instead of a backslash (\) as a separator in file paths
    this function replaces all forward slashes in a string with backslashes
    */
    std::string adjusted_path; // stores new path

    for(auto c:path) // for every character in the given path
    {
        if (c == '\\') // if that character is a forward slash
            adjusted_path += "/"; // add a backslash to the new path instead (\\ is the control character for backslash in an std::string or char *)
        else
            adjusted_path += c; // otherwise add character to new path
    }
    // removes any troublesome double slashes
    if(adjusted_path.size() > 0)
    {
        for(unsigned int i = 0; i < adjusted_path.size() - 1; i++)
        {
            if(adjusted_path.substr(i, 2) == "//")
            {
                adjusted_path.replace(i, 2, "/");
            }
        }
    }

    path = adjusted_path; // set path to new path
}

inline bool file_exists(const std::string name)
{
    // checks if a file exists
    std::ifstream f(name.c_str());
    return f.good();
}

std::vector<std::string> explode(std::string s, char c)
{
    /*
    splits a string on c
    for example, explode("list,of,words",',') returns {"list","of","words"}
    function ignores any instances of c inside double quotes
    */

	std::string buffer; // stores current word
	std::vector<std::string> v; // output
	bool track = true; // keeps track of whether the loop is in a double quote pair or not

	for(auto n:s) // iterate through s
	{
	    if(n == '"')
            track = !track; // we have just entered/left a quote pair
	    if(!track)
        {
            buffer += n; // if in quote pair, add n to buffer regardless
            continue; // skip any further comparisons
        }
		if(n != c)
            buffer += n; // if n is not c, add to buffer
        else if(n == c && buffer != "" && track)
        {
            v.push_back(buffer); // if n is c, add buffer to output
            buffer = ""; // reset buffer
        }
	}
	if(buffer !=  "") // if there is something in the buffer
        v.push_back(buffer); // add whatever buffer is to output

	return v;
}

void eliminate_whitespace(std::string &str)
{
    std::string s;
    bool eliminate = true;

    for(auto c:str)
    {
        if(c == '=') // stop removing whitespace
            eliminate = false;
        else if(c == ';') // start removing whitespace again
            eliminate = true;

        if(!(c == ' ' && eliminate))
            s += c; // if char is not whitespace, add to buffer string
    }

    str = s;
}

std::string mod_abs_path(std::string abspath, std::string relpath)
{
    // abspath must have a '\' at the end
    while(relpath.substr(0,3) == "../") // if relative path has 'go up a level', go up a level
    {
        size_t pos = abspath.find_last_of('/', abspath.size() - 2);

        abspath.erase(abspath.begin()+pos, abspath.end() - 1); // by deleting the last part of the absolute path

        relpath.erase(0,3); // remove the up one level specifier
    }
    std::string buffer = abspath + relpath; // add the relative path to the absolute path
    return buffer;
}

#ifdef SYSTEM_IS_LINUX
std::string modify_for_grep(std::string pdfid, int outopts_size)
{
    std::string output;

    pdfid = pdfid.erase(pdfid.size() - outopts_size);

    for(char c:pdfid)
    {
        if(c != '"') output += c;
    }
    while(output.back() == ' ')
        output.pop_back();

    output.insert(1, "]");
    output = "ps ax | grep \"[" + output + "\"";
    return output;
}

std::string get_stdout(FILE * pFile)
{
    char buffer[256];
    std::string filecontents;

    if(!pFile)
    {
        std::cout << "popen() failed" << std::endl;
        return "";
    }
    while(!feof(pFile))
    {
        if(fgets(buffer, 256, pFile) != NULL)
            filecontents += buffer;
    }
    return filecontents;
}
#endif

int execute_command(std::string compile, std::string bib, std::string openpdf, std::string outopts, std::string output_viewer)
{
    // prints the commands about to be executed, for checking mistakes/bugs etc
    std::cout << std::endl;
    std::cout << "LaTeX compilation command:" << std::endl;
    std::cout << compile << "\n" << std::endl;
    std::cout << "Bibliography manager command:" << std::endl;
    std::cout << bib << "\n" << std::endl;
    std::cout << "Open with command:" << std::endl;
    std::cout << openpdf << "\n" << std::endl;

    std::cout << "Process complete, executing commands..." << std::endl;

    std::cout << "==============================================================================\n" << std::endl;

    // convert std::string commands to char*
    char *compstr = &compile[0u];
    char *bibstr = &bib[0u];
    char *pdfstr = &openpdf[0u];

    #ifdef SYSTEM_IS_WINDOWS
        //{
        // Win32 witchcraft
        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof si ;

        int rc;

        if(compile != "")
        {
            // run the latex engine command string
            rc = CreateProcess(NULL, compstr, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
            WaitForSingleObject( pi.hProcess, INFINITE ); // wait for process to complete
            CloseHandle( pi.hProcess ); // remove handle to process
            CloseHandle( pi.hThread ); // remove thread

            if(!rc) // if return command is not 0, an error has occurred
            {
                std::cout << "\nError: document compilation failed\n" << std::endl;
                return 255; // IT'S ONE FIRE!!!! RUN FOR IT!!!!! ABORT! ABORT!
            }
        }

        if(bib != "") // if bibliography in use
        {
            // run the bibliography engine command string
            rc = CreateProcess(NULL, bibstr, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

            WaitForSingleObject( pi.hProcess, INFINITE ); // wait around for it do so stuff
            CloseHandle( pi.hProcess ); // clean up afterwards
            CloseHandle( pi.hThread );

            if(!rc) // ERROR
                std::cout << "\nError: bibliography manager call failed\n" << std::endl;

            // run the latex engine again to make sure references are happy
            rc = CreateProcess(NULL, compstr, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

            WaitForSingleObject( pi.hProcess, INFINITE ); // wait
            CloseHandle( pi.hProcess ); // clean
            CloseHandle( pi.hThread );
        }
        if(openpdf != "") // if the output file should be opened in something
        {
            // execute program
            rc = CreateProcess(NULL, pdfstr, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

            // the process only logs as complete when the program is closed, so don't wait around for this one
            CloseHandle( pi.hProcess ); // clean
            CloseHandle( pi.hThread );

            if(!rc)
                std::cout << "\nError: failed to open file\n" << std::endl;
        }
        //}
    #endif

    #ifdef SYSTEM_IS_LINUX

        if(compile != "")
        {
            system(compstr);
        }

        if(bib != "")
        {
            system(bibstr);
            system(compstr);
        }

        if(openpdf != "")
        {
            std::string grepstring = modify_for_grep(openpdf, outopts.size());

            FILE * grepres = popen(grepstring.c_str(), "r");

            std::string grep_output = get_stdout(grepres);

            pclose(grepres);

            if(grep_output == "")
            {
                system(pdfstr);
            }
            else if(refresh_viewer)
            {
                std::string pid_command = "pidof " + output_viewer;
                FILE * pidof = popen(pid_command.c_str(), "r");

                std::string pids_out = get_stdout(pidof);

                pclose(pidof);

                std::vector<std::string> pidlist = explode(pids_out, ' ');

                for(auto pid:pidlist)
                {
                    std::string kill_command = "kill -1 " + pid;
                    system(kill_command.c_str());
                }
            }
        }

    #endif

    return 0;
}

std::string remove_carriage_return(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    return s;
}

int parse_file(std::string dir, std::string file)
{
    // dir must have a '\' at the end, this is added automatically in main()
    std::ifstream ifile;
    std::vector<std::string> flineargs;
    std::string line, engine, bibengine, options, master, compcall, bibcall, openpdfcall, biboptions, outext, openwith, outopts;
    bool otherargs = false; // set to true if anything other than master is specified, for detecting redundant options when master is specified

    std::string texpath = dir + file; // full path to file to be compiled

    ifile.open(texpath); // open file to be compiled (maybe)

    std::cout << "Reading first line of '" << texpath << "'" << std::endl;
    getline(ifile, line); // read the first line

    ifile.close(); // close file, all the data we need has been read

    eliminate_whitespace(line); // remove whitespace, excluding that between = and ;

    line.erase(0,1); // discard first character. if first line is useful, this will be a %

    line = remove_carriage_return(line); // remove carriage return from the line to make linux-safe

    flineargs = explode(line, ';'); // split arguments up

    for(std::string arg:flineargs) // for each specifier-value pair...
    {
        if(arg.substr(0,7) == "master=") // redirect the program to a master file
        {
            master = arg.substr(7);
            std::cout << "Found specifier for master file: '" << master << "'" << std::endl;
        }
        else if(arg.substr(0,7) == "engine=") // LaTeX engine, e.g. xelatex, pdflatex
        {
            otherargs = true;
            engine = arg.substr(7);
            std::cout << "Found specifier for LaTeX engine: '" << engine << "'" << std::endl;
        }
        else if(arg.substr(0,4) == "bib=") // bibliography engine, e.g. biber, bibtex
        {
            otherargs = true;
            bibengine = arg.substr(4);
            std::cout << "Found specifier for bibliography engine: '" << bibengine << "'" << std::endl;
        }
        else if(arg.substr(0,8) == "options=") // options to pass to LaTeX engine
        {
            otherargs = true;
            options = arg.substr(8);
            std::cout << "Found specifier for LaTeX compiler options: '" << options << "'" << std::endl;
        }
        else if(arg.substr(0,11) == "biboptions=") // options to pass to bibliography engine
        {
            otherargs = true;
            biboptions = arg.substr(11);
            std::cout << "Found specifier for bibliography engine options: '" << biboptions << "'" << std::endl;
        }
        else if(arg.substr(0,7) == "outext=") // extension of output file
        {
            otherargs = true;
            outext = arg.substr(7);
            std::cout << "Found specifier for output file extension: '" << outext << "'" << std::endl;
        }
        else if(arg.substr(0,9) == "openwith=") // file to open output file with (can be 'none')
        {
            otherargs = true;
            openwith = arg.substr(9);
            std::cout << "Found specifier for program to open output with: '" << openwith << "'" << std::endl;
        }
        else if(arg.substr(0,11) == "outoptions=") // options to pass to the above program (can be 'none')
        {
            otherargs = true;
            outopts = arg.substr(11);
            std::cout << "Found specifier for output viewer options: '" << outopts << "'" << std::endl;
        }
        else // that's all this program accepts
        {
            std::cout << "Unknown specifier key '" << arg << "', ignoring..." << std::endl;
        }
    }

    // adding ability to set default master
    // this seems like a very bad idea and I can't think of any way it could be useful, but here you go anyway
    if(master == "" && master != default_master)
    {
        std::cout << "No specifier for master found, defaulting to '" << default_master << "'" << std::endl;
        master = default_master;
    }
    if(master == dont_use_specvalue) // overwrite default value
        master = "";

    sanitise_path(master); // replace forward slashes with backslashes

    if(master != "" && otherargs)
    {
        // warn user that they have specified compiler etc for this file in addition to a master file -
        // i.e. the things they have specified will be totally ignored unless the master file does not exist
        std::cout << "\nWarning: specifiers found for master file and others - ignoring other specifiers\n" << std::endl;
    }

    if(master != "")
    {
        std::cout << "Looking for master file..." << std::endl;
        std::string newpath = mod_abs_path(dir, master); // work out the path of the master file

        size_t slashpos = newpath.find_last_of('/'); // the name of the master file is all characters after the last slash in the path

        if(file_exists(newpath))
        {
            std::cout << "Found master file, parsing master file now...\n" << std::endl;
            // if the master file does exist (yay), recursively call this function again and then discard this instance
            parse_file(newpath.substr(0,slashpos+1), newpath.substr(slashpos + 1));
            return 0;
        }
        // master file does not exist, carry on trying to compile this file
        // other specifiers come back into play here
        std::cout << "\nWarning: master file '" << newpath << "' not found, attempting to compile current file...\n" << std::endl;

    }
    std::cout << std::endl;

    //{
	// set defaults for all values
    if(engine == "" && engine != default_engine)
    {
        std::cout << "No specifier for TeX\\LaTeX engine found, defaulting to '" << default_engine << "'" << std::endl;
        engine = default_engine;
    }
    if(options == "" && options != default_options)
    {
        std::cout << "No specifier for engine options found, defaulting to '" << default_options << "'" << std::endl;
        options = default_options;
    }
    if(bibengine == "" && bibengine != default_bib)
    {
        std::cout << "No specifier for bibliography engine found, defaulting to '" << default_bib << "'" << std::endl;
        bibengine = default_bib;
    }
    if(biboptions == "" && biboptions != default_biboptions)
    {
        std::cout << "No specifier for  found, defaulting to '" << default_biboptions << "'" << std::endl;
        biboptions = default_biboptions;
    }
    if(outext == "" && outext != default_outext)
    {
        std::cout << "No specifier for output extension found, defaulting to '" << default_outext << "'" << std::endl;
        outext = default_outext;
    }
    if(openwith == "" && openwith != default_openwith)
    {
        std::cout << "No specifier for output viewer found, defaulting to '" << default_openwith << "'" << std::endl;
        openwith = default_openwith;
    }
    if(outopts == "" && outopts != default_outoptions)
    {
        std::cout << "No specifier for output viewer options found, defaulting to '" << default_outoptions << "'" << std::endl;
        outopts = default_outoptions;
    }
    //}

	// if the value is equal to dont_use_specvalue, overwrite the default
    if(options == dont_use_specvalue)
        options = "";
    if(biboptions == dont_use_specvalue)
        biboptions = "";
    if(outext == dont_use_specvalue)
        outext = "";
    if(outopts == dont_use_specvalue)
        outopts = "";

	// change all forward slashes to backslashes
    sanitise_path(openwith);
    sanitise_path(options);
    sanitise_path(biboptions);
    sanitise_path(outopts);

    #ifdef SYSTEM_IS_LINUX
    outopts += " &";
    #endif

    std::cout << std::endl;

    if(dir != "")
        dir.erase(dir.size() - 1); // remove the slash from the end of the directory path

    std::cout << "Compiling optional arguments..." << std::endl;

    #ifdef COMPILER_IS_MIKTEX
    // --aux-directory is only used by MiKTeX
    if(options.find("-aux-directory=") == std::string::npos)
    {
        // tell LaTeX engine where the working directory is, if the user has not manually specified this
        options += " --aux-directory=\"" + dir + "\"";
    }
    //
    #endif

    if(options.find("-output-directory=") == std::string::npos)
    {
        // tell LaTeX engine where the working directory is, if the user has not manually specified this
        options += " --output-directory=\"" + dir + "\"";
    }

    if(biboptions.find("-output-directory=") == std::string::npos && bibengine == "biber")
    {
        // tell bibliography engine where the working directory is, if the user has not manually specified this
        // biber is special and uses --output-directory instead of --include-directory
        biboptions += " --output-directory=\"" + dir + "\"";
    }
    else if(biboptions.find("-include-directory=") == std::string::npos)
    {
        // tell bibliography engine where the working directory is, if the user has not manually specified this
        //biboptions += " --include-directory=\"" + dir + "\"";
    }

    // finds position of the file extension - the last dot in the path
    // used because bibliography engines like to be given the name part only - e.g. for main.tex the bib engine just wants 'main'
    size_t dotpos = texpath.find_last_of('.');
    size_t shortdotpos = file.find_last_of('.'); // biber likes to be given '--output-directory' and JUST the filename

    // assemble the LaTeX engine command from the various bits
    if(engine != "" && engine != dont_use_specvalue) // only do this is the value is not empty and not dont_use_specvalue
        compcall = engine + " --halt-on-error " + options + " \"" + texpath + "\"";

    // assemble the call to the program to open the output file with
    // the output file will be the file's name part plus whatever extension the user is using
    if(openwith != "" && openwith != dont_use_specvalue) // if user has specified to not open the output file in anything, this call will be empty
        openpdfcall = "\"" + openwith + "\" \"" + texpath.substr(0,dotpos) + outext + "\" " + outopts;

    if(bibengine != "" && bibengine != dont_use_specvalue) // if a bibliography engine has been specified, construct the call
    {
        bibcall = file.substr(0,shortdotpos); // strip the file extension from the file path

        std::cout << "Compiling bibliography information..." << std::endl;

        bibcall = bibengine + " " + biboptions + " \"" + bibcall + "\""; // construct call
    }

    execute_command(compcall, bibcall, openpdfcall, outopts, openwith); // execute!

    return 0;
}

int main(int argc, char *argv[])
{
    const char *homedir;
    std::cout << "This is TeXbuild v" << version << "\n" << std::endl;

    #ifdef SYSTEM_IS_LINUX
        // retrieves the home directory on linux
        if ((homedir = getenv("HOME")) == NULL)
        {
            homedir = getpwuid(getuid())->pw_dir;
        }
        config_path = std::string(homedir) + "/.texbuild/";
    #elif #defined(SYSTEM_IS_WINDOWS)
        // retrieves the home directory on windows
        homedir = getenv("USERPROFILE");
        config_path = std::string(homedir) + "/AppData/Roaming/TeXbuild/";
    #endif

    if (argc != 3)
    {
        std::cout << "Error: TeXbuild takes two arguments only" << std::endl;
        std::cout << "See documentation for details" << std::endl;
        std::cout << "Exiting TeXbuild..." << std::endl;
        return 1;
    }

    std::string namepart = argv[2];
    // path must be absolute not relative
    std::string directory = argv[1];

    sanitise_path(directory); // change forward slashes to backslashes

    if(directory != "") // add a slash to the end of the path. since the path given cannot have a slash at the end, this means i can be sure there will always be a terminating slash
    {
        if(directory.back() != '/')
            directory += '/';
    }

    if(!file_exists(directory + namepart))
    {
        std::cout << "Error: file '" << directory << namepart << "' does not exist" << std::endl;
        std::cout << "See documentation for details" << std::endl;
        std::cout << "Exiting TeXbuild..." << std::endl;
        return 2;
    }

    #ifdef USE_CONFIG_FILE_DEFAULTS

    read_config_file(); // get defaults from config.txt

    #endif

    parse_file(directory, namepart);

    return 0;
}
