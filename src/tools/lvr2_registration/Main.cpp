/**
 * Copyright (c) 2019, University Osnabrück
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University Osnabrück nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL University Osnabrück BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Main.cpp
 *
 *  @date Apr 25, 2019
 *  @author Malte Hillmann
 */

// external includes
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <chrono>

// internal includes
#include <lvr2/io/ModelFactory.hpp>
#include <lvr2/io/IOUtils.hpp>
#include <lvr2/registration/SlamAlign.hpp>

using namespace lvr2;
using namespace std;
using boost::filesystem::path;

string format_name(string prefix, int index, string suffix, int number_length = -1)
{
    string num = to_string(index);
    if (number_length != -1)
    {
        num = string(number_length - num.length(), '0') + num;
    }
    return prefix + num + suffix;
}

int main(int argc, char** argv)
{
    // =============== parse options ===============
    bool    doLoopClosing = false;
    bool    doGraphSlam = false;
    double  slamMaxDistance = 25;
    int     slamIterations = 50;

    double  icpMaxDistance = 25;
    int     icpIterations = 50;

    double  minDistance = -1;
    double  maxDistance = -1;
    double  reduction = -1;

    bool    trustPose = false;
    bool    metascan = false;

    double  epsilon = 0.00001;
    bool    quiet = false;

    int start = -1;
    int end = -1;
    string format = "uos";
    path dir;
    bool help;

    try
    {
        using namespace boost::program_options;

        options_description visible_options("OPTIONS");
        visible_options.add_options()
        ("start,s", value<int>(&start)->default_value(start), "The first scan to process.\n-1 (default): search for first scan")
        ("end,e", value<int>(&end)->default_value(end), "The last scan to process.\n-1 (default): continue until no more scan found")
        ("format,f", value<string>(&format)->default_value(format), "The format to use.\navailable formats are listed <somewhere>")
        ("icpIterations,i", value<int>(&icpIterations)->default_value(icpIterations), "Number of iterations for ICP")
        ("icpMaxDistance,d", value<double>(&icpMaxDistance)->default_value(icpMaxDistance), "The maximum distance between two points during ICP")
        ("slamIterations,I", value<int>(&slamIterations)->default_value(slamIterations), "Number of iterations for SLAM")
        ("slamMaxDistance,D", value<double>(&slamMaxDistance)->default_value(slamMaxDistance), "The maximum distance between two points during SLAM")
        ("reduction,r", value<double>(&reduction)->default_value(reduction), "The Voxel size for Voxel based reduction. -1 for no reduction")
        ("min,m", value<double>(&minDistance)->default_value(minDistance), "Ignore all Points closer than <value> to the origin of the scan\n-1 (default): No filter")
        ("max,M", value<double>(&maxDistance)->default_value(maxDistance), "Ignore all Points farther away than <value> from the origin of the scan\n-1 (default): No filter")
        ("epsilon", value<double>(&epsilon)->default_value(epsilon), "The desired epsilon difference between two error values")
        ("loop,L", bool_switch(&doLoopClosing), "Use simple Loop Closing")
        ("graph,G", bool_switch(&doGraphSlam), "Use complex Loop Closing with GraphSLAM")
        ("trustPose,p", bool_switch(&trustPose), "Use the unmodified Pose for ICP. Useful for GPS Poses")
        ("metascan", bool_switch(&metascan), "Match scans to the combined pointcloud of all previous scans")
        ("quiet,q", bool_switch(&quiet), "Hide detailed output and only show results")
        ("help,h", bool_switch(&help), "Print this help")
        ;

        options_description hidden_options("hidden_options");
        hidden_options.add_options()
        ("dir", value<path>(&dir))
        ;

        positional_options_description pos;
        pos.add("dir", 1);

        options_description options("options");
        options.add(visible_options).add(hidden_options);

        variables_map variables;
        store(command_line_parser(argc, argv).options(options).positional(pos).run(), variables);
        notify(variables);

        if (help)
        {
            cout << "The Scan Registration Tool" << endl;
            cout << "Usage: " << endl;
            cout << "\tlvr2_registration [OPTIONS] <dir>" << endl;
            cout << endl;
            visible_options.print(cout);
            cout << endl;
            cout << "<dir> is the directory to search scans in" << endl;
            return EXIT_SUCCESS;
        }
        if (variables.count("dir") != 1)
        {
            throw error("Missing <dir> Parameter");
        }
    }
    catch (const boost::program_options::error& ex)
    {
        std::cerr << ex.what() << endl;
        std::cerr << endl;
        std::cerr << "Use '--help' to see the list of possible options" << endl;
        return EXIT_FAILURE;
    }

    // =============== parse format ===============
    string format_prefix;
    string format_suffix;
    int format_number_length;

    size_t split_point;
    if ((split_point = format.find('%')) != string::npos)
    {
        format_prefix = format.substr(0, split_point);
        split_point += 1; // ignore %

        size_t second_split = format.find('i', split_point);
        if (second_split == split_point)
        {
            format_number_length = -1;
        }
        else
        {
            format_number_length = stoi(format.substr(split_point, second_split - split_point));
        }

        format_suffix = format.substr(second_split + 1);
    }
    else
    {
        // TODO: map formats
        format_prefix = "scan";
        format_suffix = ".3d";
        format_number_length = 3;
    }

    // =============== search scans ===============
    if (start == -1)
    {
        for (int i = 0; i < 100; i++)
        {
            path file = dir / format_name(format_prefix, i, format_suffix, format_number_length);
            if (exists(file))
            {
                start = i;
                cout << "First scan: " << file.filename() << endl;
                break;
            }
        }
        if (start == -1)
        {
            cerr << "Could not find a starting scan. are you using the right format?" << endl;
            return EXIT_FAILURE;
        }
    }

    // make sure all scan and pose files are in the directory
    for (int i = start; end == -1 || i <= end; i++)
    {
        path file = dir / format_name(format_prefix, i, format_suffix, format_number_length);
        if (!exists(file))
        {
            if (end != -1 || i == start)
            {
                cerr << "Missing scan " << file.filename() << endl;
                return EXIT_FAILURE;
            }
            end = i - 1;
            cout << "Last scan: \"" << format_name(format_prefix, end, format_suffix, format_number_length) << '"' << endl;
            break;
        }
        file.replace_extension("pose");
        if (!exists(file))
        {
            cerr << "Missing pose file " << file.filename() << endl;
            return EXIT_FAILURE;
        }
    }

    vector<ScanPtr> scans;
    scans.reserve(end - start);

    for (int i = start; i <= end; i++)
    {
        path file = dir / format_name(format_prefix, i, format_suffix, format_number_length);
        auto model = ModelFactory::readModel(file.string());

        file.replace_extension("pose");
        Matrix4d pose = getTransformationFromPose(file);

        scans.push_back(make_shared<Scan>(model->m_pointCloud, pose));
    }

    SlamAlign align(scans);

    align.setDoLoopClosing(doLoopClosing);
    align.setDoGraphSlam(doGraphSlam);
    align.setSlamMaxDistance(slamMaxDistance);
    align.setSlamIterations(slamIterations);

    align.setIcpMaxDistance(icpMaxDistance);
    align.setIcpIterations(icpIterations);

    align.setMinDistance(minDistance);
    align.setMaxDistance(maxDistance);
    align.setReduction(reduction);

    align.setTrustPose(trustPose);
    align.setMetascan(metascan);

    align.setEpsilon(epsilon);
    align.setQuiet(quiet);

    auto start_time = chrono::steady_clock::now();

    align.match();

    auto required_time = chrono::steady_clock::now() - start_time;
    cout << "SLAM finished in " << required_time.count() / 1e9 << " seconds" << endl;

    for (int i = start; i <= end; i++)
    {
        path file = dir / format_name(format_prefix, i, format_suffix, format_number_length);
        file.replace_extension("frames");

        scans[i - start]->writeFrames(file.string());
    }

    return EXIT_SUCCESS;
}
