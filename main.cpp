/****************************************************************************
** Copyright (c) 2017 Adrian Schneider, AOT AG
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
*****************************************************************************/


#include <iostream>
#include <iomanip>
#include <string>
#include <vtkAlgorithm.h>

#include "vtkMeshRoutines.h"
#include "vtkDicomRoutines.h"

// Note: In order to safe memory, smart-pointers were not used for certain
//       objects. This has the advantage that memory blocks can be released
//       within the function body.

using namespace std;

struct Dicom2MeshSettings
{
    string pathToDicomDirectory;
    bool pathToDicomSet = false;
    string outputFilePath = "mesh.stl";
    bool pathToOutputSet = false;
    int isoValue = 400; // Hard Tissue
    bool setOriginToCenterOfMass = false;
    bool enableMeshReduction = false;
    float reductionRate = 0.5;
    bool extracOnlyBigObjects = false;
    float nbrVerticesRatio = 0.1;
    bool enableSmoothing = false;
};

void myVtkProgressCallback(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData)
{
    // display progress in terminal
    vtkAlgorithm* filter = static_cast<vtkAlgorithm*>(caller);
    char* task = static_cast<char*>(clientData);
    cout << "\33[2K\r"; // erase line
    cout << task << ": ";
    if( filter->GetProgress() > 0.999 )
        cout << "done";
    else
        cout << std::fixed << std::setprecision( 1 )  << filter->GetProgress() * 100 << "%";
    cout << flush;
}

void showUsage()
{
    cout << "How to use dicom2Mesh:" << endl << endl;

    cout << "Minimum example. This creates a mesh file called mesh.stl by using a iso value of 400 (makes bone visible)" << endl;
    cout << "> dicom2mesh -i pathToDicomDirectory " << endl << endl;

    cout << "This creates a mesh file called abc.stl by using a iso value of 700" << endl;
    cout << "> dicom2mesh -i pathToDicomDirectory  -o abc.stl  -t 700 " << endl << endl;

    cout << "This creates a mesh with a reduced number of polygons by half" << endl;
    cout << "> dicom2mesh -i pathToDicomDirectory  -r" << endl << endl;

    cout << "This creates a mesh with a reduced number of polygons by 80%" << endl;
    cout << "> dicom2mesh -i pathToDicomDirectory  -r 0.8" << endl << endl;

    cout << "This creates a mesh where small connected objects are removed. In particular, only connected objects with a minimum number of vertices of 20% of the object with the most vertices are part of the result." << endl;
    cout << "> dicom2mesh -i pathToDicomDirectory  -e  0.2" << endl << endl;

    cout << "This creates a mesh which is shifted to the coordinate system origin." << endl;
    cout << "> dicom2mesh -i pathToDicomDirectory  -c" << endl << endl;

    cout << "This creates a mesh which is smoothed." << endl;
    cout << "> dicom2mesh -i pathToDicomDirectory  -s" << endl << endl;

    cout << "Arguments can be combined." << endl << endl;
}

string getSettingsAsString( const Dicom2MeshSettings& settings )
{
    string ret = "Dicom2Mesh Settings\n-------------------\n";
    ret.append("Input directory: "); ret.append(settings.pathToDicomDirectory); ret.append("\n");
    ret.append("Output file path: "); ret.append(settings.outputFilePath); ret.append("\n");
    ret.append("Surface segementation: "); ret.append( to_string(settings.isoValue )); ret.append("\n");
    ret.append("Mesh reduction: ");
    if(settings.enableMeshReduction)
    {
        ret.append("enabled (rate="); ret.append( to_string(settings.reductionRate )); ret.append(")\n");
    }
    else
    {
        ret.append("disabled\n");
    }
    ret.append("Mesh smoothing: ");
    if(settings.enableSmoothing)
    {
        ret.append("enabled\n");
    }
    else
    {
        ret.append("disabled\n");
    }
    ret.append("Mesh centering: ");
    if(settings.setOriginToCenterOfMass)
    {
        ret.append("enabled\n");
    }
    else
    {
        ret.append("disabled\n");
    }
    ret.append("Mesh filtering: ");
    if(settings.extracOnlyBigObjects)
    {
        ret.append("enabled (size-ratio="); ret.append( to_string(settings.nbrVerticesRatio )); ret.append(")\n");
    }
    else
    {
        ret.append("disabled\n");
    }

    return ret;
}

bool parseSettings( const int& argc, char* argv[], Dicom2MeshSettings& settings )
{
    for( int a = 0; a < argc; a++ )
    {
        string cArg( argv[a] );

        if( cArg.compare("-i") == 0 )
        {
            // next argument is path to dicom directory
            a++;
            if( a < argc )
            {
                settings.pathToDicomDirectory = argv[a];
                settings.pathToDicomSet = true;
            }
            else
            {
                showUsage();
                return false;
            }
        }
        else if( cArg.compare("-o") == 0 )
        {
            // next argument is file path to mesh output
            a++;
            if( a < argc )
            {
                settings.outputFilePath = argv[a];
                settings.pathToOutputSet = true;
            }
            else
            {
                showUsage();
                return false;
            }
        }
        else if( cArg.compare("-t") == 0 )
        {
            // next argument is iso value (int)
            a++;
            if( a < argc )
            {
                settings.isoValue = stoi( string(argv[a]) );
            }
            else
            {
                showUsage();
                return false;
            }
        }
        else if( cArg.compare("-h") == 0 )
        {
            showUsage();
            return true;
        }
        else if( cArg.compare("-r") == 0 )
        {
            settings.enableMeshReduction = true;
            // next argument is reduction (float)
            a++;
            if( a < argc ) // default value is 0.5
                settings.reductionRate = stof( string(argv[a]) );
        }
        else if( cArg.compare("-e") == 0 )
        {
            settings.extracOnlyBigObjects = true;
            // next argument is size ratio (float)
            a++;
            if( a < argc ) // default value is 0.1
                settings.nbrVerticesRatio = stof( string(argv[a]) );
        }
        else if( cArg.compare("-c") == 0 )
        {
            settings.setOriginToCenterOfMass = true;
        }
        else if( cArg.compare("-s") == 0 )
        {
            settings.enableSmoothing = true;
        }
    }

    if( !settings.pathToDicomSet )
    {
        cerr << "Path to DICOM directory missing" << endl << "> dicom2mesh -i pathToDicom" << endl;
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    //****** Parse arguments *******//
    Dicom2MeshSettings settings;
    if( !parseSettings( argc, argv, settings) )
        return -1;

    cout << endl << getSettingsAsString( settings ) << endl;
    //******************************//

    vtkSmartPointer<vtkCallbackCommand> progressCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    progressCallback->SetCallback(myVtkProgressCallback);

    //******** Read DICOM *********//
    vtkImageData* imgData = VTKDicomRoutines::loadDicomImage( settings.pathToDicomDirectory, progressCallback );
    vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>( VTKDicomRoutines::dicomToMesh( imgData, settings.isoValue, progressCallback ) );
    imgData->Delete();
    //******************************//

    if( mesh->GetNumberOfCells() == 0 )
    {
        cerr << "No mesh could be created. Wrong DICOM or wrong iso value" << endl;
        return -1;
    }

    //***** Mesh post-processing *****//
    if( settings.setOriginToCenterOfMass )
        VTKMeshRoutines::moveMeshToCOSCenter( mesh );

    if( settings.enableMeshReduction )
    {
        // check reduction rate
        if( settings.reductionRate < 0.0 || settings.reductionRate > 1.0 )
            cout << "Reduction skipped due to invalid reductionRate " << settings.reductionRate << " where a value of 0.0 - 1.0 is expected." << endl;
        else
           VTKMeshRoutines:: meshReduction( mesh, settings.reductionRate, progressCallback );
    }

    if( settings.extracOnlyBigObjects )
    {
        if( settings.nbrVerticesRatio < 0.0 || settings.nbrVerticesRatio > 1.0 )
            cout << "Smoothing skipped due to invalid reductionRate " << settings.nbrVerticesRatio << " where a value of 0.0 - 1.0 is expected." << endl;
        else
            VTKMeshRoutines::removeSmallObjects( mesh, settings.nbrVerticesRatio );
    }

    if( settings.enableSmoothing )
    {
        VTKMeshRoutines::smoothMesh( mesh, 20, progressCallback );
    }

    //********************************//

    // check if obj or stl
    string::size_type idx = settings.outputFilePath.rfind('.');
    if( idx != string::npos )
    {
        string extension = settings.outputFilePath.substr(idx+1);

        if( extension == "obj" )
            VTKMeshRoutines::exportAsObjFile( mesh, settings.outputFilePath );
        else if( extension == "stl" )
            VTKMeshRoutines::exportAsStlFile( mesh, settings.outputFilePath );
        else
            cerr << "Unknown file type" << endl;
    }
    else
    {
        cerr << "No Filename." << endl;
    }

    return 0;
}



