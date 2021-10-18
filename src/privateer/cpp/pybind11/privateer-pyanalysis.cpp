// Library for the YSBL program Privateer (PRogramatic Identification of Various Anomalies Toothsome Entities Experience in Refinement)
// Licence: LGPL - Please check Licence.txt for details.
//
// 2013-
// York Structural Biology Laboratory
// The University of York

#include "privateer-pyanalysis.h"

// using namespace pybind11::literals;

#define DBG std::cout << "[" << __FUNCTION__ << "] - "

///////////////////////////////////////////////// Class GlycosylationComposition ////////////////////////////////////////////////////////////////////
privateer::pyanalysis::GlycosylationComposition::GlycosylationComposition(std::string& path_to_model_file, std::string& path_to_mtz_file, std::string& input_column_fobs_user, int nThreads, float ipradius, std::string expression_system, bool debug_output) 
{
    this->debug_output = debug_output;
    this->read_from_file ( path_to_model_file, expression_system, debug_output );
    privateer::pyanalysis::XRayData experimental_data(path_to_mtz_file, path_to_model_file, input_column_fobs_user, ipradius, nThreads, debug_output);
    this->update_with_experimental_data(experimental_data);
};

privateer::pyanalysis::GlycosylationComposition::GlycosylationComposition(std::string& path_to_model_file, std::string& path_to_mrc_file, float resolution, int nThreads, float ipradius, std::string expression_system, bool debug_output) 
{
    this->debug_output = debug_output;
    this->read_from_file ( path_to_model_file, expression_system, debug_output );
    privateer::pyanalysis::CryoEMData experimental_data(path_to_mrc_file, path_to_model_file, resolution, ipradius, nThreads, debug_output);
    this->update_with_experimental_data(experimental_data);
};

void privateer::pyanalysis::GlycosylationComposition::read_from_file( std::string path_to_model_file, std::string expression_system, bool debug_output ) {

    if(path_to_model_file == "undefined")
    {
        throw std::invalid_argument( "No path was provided for model file input! Aborting." );
    }

    this->path_to_model_file = path_to_model_file;
    this->expression_system = expression_system;
    
    clipper::MiniMol mmol;
    clipper::MMDBfile mfile;
    clipper::String path_to_model_file_clipper = path_to_model_file;

    privateer::util::read_coordinate_file_mtz(mfile, mmol, path_to_model_file_clipper, true);

    const clipper::MAtomNonBond& manb = clipper::MAtomNonBond( mmol, 1.0 ); // was 1.0

    this->mgl = clipper::MGlycology(mmol, manb, debug_output, expression_system);
    
    std::vector<clipper::MGlycan> list_of_glycans = mgl.get_list_of_glycans();
    
    clipper::Atom_list mainAtoms;
    clipper::Atom_list ligandAtoms;
    clipper::Atom_list allAtoms;

    std::vector<std::pair< clipper::String , clipper::MSugar> > ligandList; // we store the Chain ID and create an MSugar to be scored
    std::vector<clipper::MMonomer> sugarList; // store the original MMonomer

    for ( int p = 0; p < mmol.size(); p++ )
    {
        for ( int m = 0; m < mmol[p].size(); m++ )
        {
            if ( clipper::MDisaccharide::search_disaccharides(mmol[p][m].type().c_str()) != -1 ) // treat disaccharide
            {
                clipper::MDisaccharide md(mmol, manb, mmol[p][m], debug_output );
                sugarList.push_back ( mmol[p][m] );
                sugarList.push_back ( mmol[p][m] );
                clipper::String id = mmol[p].id();
                id.resize(1);

                ligandList.push_back ( std::pair < clipper::String, clipper::MSugar> (id, md.get_first_sugar()));
                ligandList.push_back ( std::pair < clipper::String, clipper::MSugar> (id, md.get_second_sugar()));

                if (( md.get_first_sugar().type_of_sugar() == "unsupported" ) || ( md.get_second_sugar().type_of_sugar() == "unsupported" ) )
                {
                    throw std::invalid_argument( "Error: strangely, at least one of the sugars in the supplied PDB file is missing required atoms. Stopping..." );
                }

                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    ligandAtoms.push_back(mmol[p][m][id]);  // add the ligand atoms to a second array
                    allAtoms.push_back(mmol[p][m][id]);
                }

            }
            else if ( !clipper::MSugar::search_database(mmol[p][m].type().c_str()) )
            {
                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    mainAtoms.push_back(mmol[p][m][id]); // cycle through atoms and copy them
                    allAtoms.push_back(mmol[p][m][id]);
                }
            }
            else // it's one of the sugars contained in the database
            {
                clipper::MSugar msug, msug_b;

                std::vector <char> conformers = privateer::util::number_of_conformers(mmol[p][m]);

                // if(debug_output)
                // {
                //     std::cout << "number of alternate conformations: " << conformers.size() << std::endl;
                // }

                int n_conf = conformers.size();

                if ( n_conf > 0 )
                {
                    if ( n_conf == 1 )
                        msug   = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[0]);
                    else
                    {
                        msug   = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[0]);
                        msug_b = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[1]);
                    }

                }
                else
                {
                    msug = clipper::MSugar(mmol, mmol[p][m], manb, debug_output);
                }

                sugarList.push_back(mmol[p][m]);
                clipper::String id = mmol[p].id();
                id.resize(1);

                ligandList.push_back(std::pair<clipper::String, clipper::MSugar> (id, msug));
                // add both conformers if the current monomer contains more than one
                if ( n_conf == 2 )
                {
                    ligandList.push_back(std::pair<clipper::String, clipper::MSugar> (id, msug_b));
                    sugarList.push_back(mmol[p][m]);
                }

                if ( msug.type_of_sugar() == "unsupported" )
                {
                    throw std::invalid_argument( "Error: strangely, at least one of the sugars in the supplied PDB file is missing required atoms. Stopping..." );
                }

                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    ligandAtoms.push_back(mmol[p][m][id]);  // add the ligand atoms to a second array
                    allAtoms.push_back(mmol[p][m][id]);
                }
            }
        }
    }

    std::vector<std::pair<clipper::String, clipper::MSugar>> ligandsOnly;
    int processedMonomers = 0;
    for (int index = 0; index < ligandList.size(); index++)
    {
        float x,y,z,maxX,maxY,maxZ,minX,minY,minZ;
        x=y=z=0.0;
        maxX=maxY=maxZ=-999999.0;
        minX=minY=minZ=999999.0;

        for (int natom = 0; natom < sugarList[index].size(); natom++)
        {
            if(sugarList[index][natom].coord_orth().x() > maxX) maxX=sugarList[index][natom].coord_orth().x(); // calculation of the sugar centre
            if(sugarList[index][natom].coord_orth().y() > maxY) maxY=sugarList[index][natom].coord_orth().y();
            if(sugarList[index][natom].coord_orth().z() > maxZ) maxZ=sugarList[index][natom].coord_orth().z();
            if(sugarList[index][natom].coord_orth().x() < minX) minX=sugarList[index][natom].coord_orth().x();
            if(sugarList[index][natom].coord_orth().y() < minY) minY=sugarList[index][natom].coord_orth().y();
            if(sugarList[index][natom].coord_orth().z() < minZ) minZ=sugarList[index][natom].coord_orth().z();
        }

        x = minX + ((maxX - minX)/2);
        y = minY + ((maxY - minY)/2);
        z = minZ + ((maxZ - minZ)/2);

        ligandList[index].second.set_rscc ( -1 );
        ligandList[index].second.set_accum_score ( -1 );
        // calculation of the mean densities of the calc (ligandmap) and weighted obs (sigmaamap) maps

        bool found_in_tree = false;
        for ( int i = 0 ; i < list_of_glycans.size() ; i++ )
        {
            std::vector < clipper::MSugar > list_of_sugars = list_of_glycans[i].get_sugars();

            for ( int j = 0 ; j < list_of_sugars.size() ; j++ )
            {
                if ( list_of_sugars[j].id().trim() == ligandList[index].second.id().trim() )
                {
                    if ( list_of_glycans[i].get_type() == "n-glycan" )
                    {
                        ligandList[index].second.set_context ( "n-glycan" );
                        
                    }
                    else if ( list_of_glycans[i].get_type() == "c-glycan" )
                    {
                        if ( ligandList[index].second.type().trim() == "MAN" ) {
                            if ( ligandList[index].second.conformation_name() == "1c4" ) {
                            ligandList[index].second.override_conformation_diag ( true );
                            }
                        }
                        ligandList[index].second.set_context ( "c-glycan" );
                    }
                    else if ( list_of_glycans[i].get_type() == "o-glycan" )
                    {
                        ligandList[index].second.set_context ( "o-glycan" );
                        
                    }
                    else if ( list_of_glycans[i].get_type() == "s-glycan" )
                    {
                        ligandList[index].second.set_context ( "s-glycan" );
                    }
                    else if ( list_of_glycans[i].get_type() == "ligand" )
                    {
                        ligandList[index].second.set_context ( "ligand" );
                    }
                    found_in_tree = true;
                    break;
                }
            }
            if ( found_in_tree ) break;
        }

        if ( !found_in_tree )
        {
            ligandList[index].second.set_context ( "ligand" );
            ligandsOnly.push_back(ligandList[index]);
        }

    
        bool occupancy_check = false;
        std::vector<clipper::MAtom> ringcomponents = ligandList[index].second.ring_members();

        for ( int i = 0 ; i < ringcomponents.size() ; i++ )
            if (privateer::util::get_altconformation(ringcomponents[i]) != ' ')
                occupancy_check = true;

        ligandList[index].second.set_occupancy_check ( occupancy_check );

        processedMonomers++;

        if(debug_output)
        {
            std::cout << std::endl;
            DBG << "Processed " << processedMonomers << "/" << ligandList.size() << " monomers..." << std::endl;
        }
    }

    this->updatedWithExperimentalData = false;
    this->ligandList = ligandList;
    this->ligandsOnly = ligandsOnly;

    auto glycanList = pybind11::list();
    for(int i = 0; i < list_of_glycans.size(); i++)
    {
        auto glycanObject = privateer::pyanalysis::GlycanStructure(mgl, i, *this);
        glycanList.append(glycanObject);
    }
    this->glycans = glycanList;

    auto ligandPyList = pybind11::list();
    if(!ligandsOnly.empty())
    {
        for(int i = 0; i < ligandsOnly.size(); i++)
        {
            // bool updatedWithExperimentalData
            auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(i, ligandsOnly, *this, updatedWithExperimentalData);
            ligandPyList.append(sugarObject);
        }
    }
    this->glycans = glycanList;
    this->ligands = ligandPyList;
    
    initialize_summary_of_detected_glycans();
}

void privateer::pyanalysis::GlycosylationComposition::initialize_summary_of_detected_glycans()
{

    this->numberOfGlycanChains = glycans.size();

    auto list = pybind11::list();
    for(int glycanID = 0; glycanID < glycans.size(); glycanID++)
    {
        auto glycanObject = glycans[glycanID].cast<privateer::pyanalysis::GlycanStructure>();
        std::string wurcsNotation = glycanObject.get_wurcs_notation();
        std::string kindOfGlycan = glycanObject.get_glycosylation_type();
        
        auto rootSummary = glycanObject.get_root_info();
        
        auto protein_glycan_linkage_torsion = glycanObject.get_protein_glycan_linkage_torsions();
        
        auto dict = pybind11::dict ("GlycanID"_a=glycanID, "WURCS"_a=wurcsNotation, "GlycosylationType"_a=kindOfGlycan, "RootInfo"_a=rootSummary, "ProteinGlycanLinkageTorsion"_a=protein_glycan_linkage_torsion, "ExperimentalData"_a=updatedWithExperimentalData);
        list.append(dict);
    }
    this->glycosylationSummary = list;
}


privateer::pyanalysis::GlycanStructure privateer::pyanalysis::GlycosylationComposition::get_glycan(const int glycanID)
{
    if(glycanID >= numberOfGlycanChains || glycanID < 0)
    {
        throw std::invalid_argument( "Provided ID is out of bounds and exceeds/inceeds number of glycans detected in the model. \nInput: " + std::to_string(glycanID) + "\tPermitted Range: [0-" + std::to_string(numberOfGlycanChains - 1) + "]");
    }

    auto glycanObject = glycans[glycanID].cast<privateer::pyanalysis::GlycanStructure>();
    return glycanObject;
}

void privateer::pyanalysis::GlycosylationComposition::update_with_experimental_data(privateer::pyanalysis::XRayData& xray_data)
{
    this->updatedWithExperimentalData = true;
    std::vector<clipper::MGlycan> list_of_glycans = mgl.get_list_of_glycans();
    std::vector<std::pair< clipper::String , clipper::MSugar> > finalLigandList = xray_data.get_finalLigandList();
    std::vector<std::pair< clipper::String , clipper::MSugar> > ligandsOnly = xray_data.get_finalLigandOnly();
    auto glycanList = pybind11::list();
    for(int i = 0; i < list_of_glycans.size(); i++)
    {
        auto glycanObject = privateer::pyanalysis::GlycanStructure(mgl, i, *this, finalLigandList);
        glycanList.append(glycanObject);
    }
    this->glycans = glycanList;

    auto ligandPyList = pybind11::list();
    if(!ligandsOnly.empty())
    {
        for(int i = 0; i < ligandsOnly.size(); i++)
        {
            auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(i, ligandsOnly, *this, updatedWithExperimentalData);
            ligandPyList.append(sugarObject);
        }
    }
    this->ligands = ligandPyList;

    initialize_summary_of_detected_glycans();
}

void privateer::pyanalysis::GlycosylationComposition::update_with_experimental_data(privateer::pyanalysis::CryoEMData& cryoem_data)
{
    this->updatedWithExperimentalData = true;
    std::vector<clipper::MGlycan> list_of_glycans = mgl.get_list_of_glycans();
    std::vector<std::pair< clipper::String , clipper::MSugar> > finalLigandList = cryoem_data.get_finalLigandList();
    std::vector<std::pair< clipper::String , clipper::MSugar> > ligandsOnly = cryoem_data.get_finalLigandOnly();
    auto glycanList = pybind11::list();
    for(int i = 0; i < list_of_glycans.size(); i++)
    {
        auto glycanObject = privateer::pyanalysis::GlycanStructure(mgl, i, *this, finalLigandList);
        glycanList.append(glycanObject);
    }
    this->glycans = glycanList;

    auto ligandPyList = pybind11::list();
    if(!ligandsOnly.empty())
    {
        for(int i = 0; i < ligandsOnly.size(); i++)
        {
            auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(i, ligandsOnly, *this, updatedWithExperimentalData);
            ligandPyList.append(sugarObject);
        }
    }
    this->ligands = ligandPyList;

    
    initialize_summary_of_detected_glycans();
}

pybind11::list privateer::pyanalysis::GlycosylationComposition::get_torsions_summary(OfflineTorsionsDatabase& importedDatabase)
{
    if(!torsions.empty())
        return torsions;
    else
    {
        auto output = pybind11::list();
        int totalGlycans = get_number_of_glycan_chains_detected();
        for(int i = 0; i < totalGlycans; i++)
        {
            privateer::pyanalysis::GlycanStructure currentGlycan = get_glycan(i);
            pybind11::list current_glycan_torsions = currentGlycan.get_torsions_summary(importedDatabase);

            auto current_glycan_torsion_info = pybind11::dict("glycanIndex"_a=i, "WURCS"_a=currentGlycan.get_wurcs_notation(), "all_torsions_in_structure"_a=current_glycan_torsions);
            output.append(current_glycan_torsion_info);
        }
        this->torsions = output;
        return output;
    }
}
///////////////////////////////////////////////// Class GlycosylationComposition END ////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////// Class GlycosylationComposition_memsafe ////////////////////////////////////////////////////////////////////
void privateer::pyanalysis::GlycosylationComposition_memsafe::read_from_file( std::string path_to_model_file, std::string expression_system, bool debug_output ) 
{
    this->debug_output = debug_output;
    if(path_to_model_file == "undefined")
    {
        throw std::invalid_argument( "No path was provided for model file input! Aborting." );
    }

    this->path_to_model_file = path_to_model_file;
    this->expression_system = expression_system;
    
    clipper::MiniMol mmol;
    clipper::MMDBfile mfile;
    clipper::String path_to_model_file_clipper = path_to_model_file;

    privateer::util::read_coordinate_file_mtz(mfile, mmol, path_to_model_file_clipper, true);

    this->mgl = clipper::MGlycology(mmol, debug_output, expression_system);

    initialize_summary_of_detected_glycans(mgl);
}
void privateer::pyanalysis::GlycosylationComposition_memsafe::initialize_summary_of_detected_glycans( clipper::MGlycology& mglObject )
{
    std::vector<clipper::MGlycan> list_of_glycans = mglObject.get_list_of_glycans();

    this->numberOfGlycanChains = list_of_glycans.size();

    auto list = pybind11::list();
    for(int i = 0; i < list_of_glycans.size(); i++)
    {
        std::string wurcsNotation = list_of_glycans[i].generate_wurcs();
        std::string kindOfGlycan = list_of_glycans[i].get_type();
        
        std::string proteinResidue = list_of_glycans[i].get_root().first.type().trim();
        std::string proteinResidueID = list_of_glycans[i].get_root().first.id().trim();
        std::string proteinChainID = list_of_glycans[i].get_chain().substr(0,1);
        std::string sugarRootChainID = list_of_glycans[i].get_root_sugar_chainID();

        auto rootSummary = pybind11::dict("ProteinResidueType"_a=proteinResidue, "ProteinResidueID"_a=proteinResidueID, "ProteinChainID"_a=proteinChainID, "RootSugarChainID"_a=sugarRootChainID);
        
        std::vector<float> torsions = list_of_glycans[i].get_glycosylation_torsions();
        auto protein_glycan_linkage_torsion = pybind11::dict ("Phi"_a=torsions[0], "Psi"_a=torsions[1]);
        
        auto dict = pybind11::dict ("GlycanID"_a=i, "WURCS"_a=wurcsNotation, "GlycosylationType"_a=kindOfGlycan, "RootInfo"_a=rootSummary, "ProteinGlycanLinkageTorsion"_a=protein_glycan_linkage_torsion);
        list.append(dict);
    }
    this->glycosylationSummary = list;
}


privateer::pyanalysis::GlycanStructure privateer::pyanalysis::GlycosylationComposition_memsafe::get_glycan(const int glycanID)
{
    if(glycanID >= numberOfGlycanChains || glycanID < 0)
    {
        throw std::invalid_argument( "Provided ID is out of bounds and exceeds/inceeds number of glycans detected in the model. \nInput: " + std::to_string(glycanID) + "\tPermitted Range: [0-" + std::to_string(numberOfGlycanChains - 1) + "]");
    }

    auto glycanObject = privateer::pyanalysis::GlycanStructure(mgl, glycanID);
    
    return glycanObject;
}

pybind11::list privateer::pyanalysis::GlycosylationComposition_memsafe::get_torsions_summary(OfflineTorsionsDatabase& importedDatabase)
{

    auto output = pybind11::list();
    int totalGlycans = get_number_of_glycan_chains_detected();
    for(int i = 0; i < totalGlycans; i++)
    {
        privateer::pyanalysis::GlycanStructure currentGlycan = this->get_glycan(i);
        pybind11::list current_glycan_torsions = currentGlycan.get_torsions_summary(importedDatabase);

        auto current_glycan_torsion_info = pybind11::dict("glycanIndex"_a=i, "WURCS"_a=currentGlycan.get_wurcs_notation(), "all_torsions_in_structure"_a=current_glycan_torsions);
        output.append(current_glycan_torsion_info);
    }

    return output;
}

///////////////////////////////////////////////// Class GlycosylationComposition_memsafe END ////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Class GlycanStructure ////////////////////////////////////////////////////////////////////
void privateer::pyanalysis::GlycanStructure::pyinit( const clipper::MGlycology& mgl, const int glycanID, privateer::pyanalysis::GlycosylationComposition& parentGlycosylationComposition)
{
    this->parentGlycosylation = parentGlycosylationComposition;
    std::vector<clipper::MGlycan> list_of_glycans = mgl.get_list_of_glycans();

    clipper::MGlycan inputGlycan = list_of_glycans[glycanID];

    this->glycan = inputGlycan;
    this->sugars_in_glycan = inputGlycan.get_sugars();
    
    this->glycanID = glycanID;
    this->numberOfSugars = inputGlycan.number_of_nodes();
    this->glycanWURCS = inputGlycan.generate_wurcs();

    auto uniqueMonosaccharidesPyList = pybind11::list();
    std::vector<std::string> uniqueMonosaccharidesVector = inputGlycan.obtain_unique_residue_codes();
    for(int i = 0; i < uniqueMonosaccharidesVector.size(); i++)
    {
        uniqueMonosaccharidesPyList.append(uniqueMonosaccharidesVector[i]);
    }
    this->uniqueMonosaccharides = uniqueMonosaccharidesPyList;
    this->numberOfGlycosidicBonds = inputGlycan.obtain_total_number_of_glycosidic_bonds();
    this->glycosylationType = inputGlycan.get_type();
    
    std::string proteinResidue = inputGlycan.get_root().first.type().trim();
    std::string proteinResidueID = inputGlycan.get_root().first.id().trim();
    std::string proteinChainID = inputGlycan.get_chain().substr(0,1);
    this->chain_root_sugar_ID = inputGlycan.get_root_sugar_chainID();
    
    auto rootSummary = pybind11::dict("ProteinResidueType"_a=proteinResidue, "ProteinResidueID"_a=proteinResidueID, "ProteinChainID"_a=proteinChainID, "RootSugarChainID"_a=chain_root_sugar_ID);
    this->rootSummary = rootSummary;

    std::vector<float> torsions = inputGlycan.get_glycosylation_torsions();
    auto protein_glycan_linkage_torsion = pybind11::dict("Phi"_a=torsions[0], "Psi"_a=torsions[1]);
    this->protein_glycan_linkage_torsion = protein_glycan_linkage_torsion;

    std::vector<clipper::MSugar> list_of_sugars = inputGlycan.get_sugars();

    auto list = pybind11::list();
    for(int i = 0; i < list_of_sugars.size(); i++)
    {
        auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(inputGlycan, i, glycanID, parentGlycosylation, *this);
        list.append(sugarObject);
    }
    this->sugars = list;

    initialize_summary_of_glycan();
}

void privateer::pyanalysis::GlycanStructure::pyinit_memsafe( const clipper::MGlycology& mgl, const int glycanID)
{
    this->parentGlycosylation = privateer::pyanalysis::GlycosylationComposition();
    std::vector<clipper::MGlycan> list_of_glycans = mgl.get_list_of_glycans();

    clipper::MGlycan inputGlycan = list_of_glycans[glycanID];

    this->glycan = inputGlycan;
    this->sugars_in_glycan = inputGlycan.get_sugars();
    
    this->glycanID = glycanID;
    this->numberOfSugars = inputGlycan.number_of_nodes();
    this->glycanWURCS = inputGlycan.generate_wurcs();

    auto uniqueMonosaccharidesPyList = pybind11::list();
    std::vector<std::string> uniqueMonosaccharidesVector = inputGlycan.obtain_unique_residue_codes();
    for(int i = 0; i < uniqueMonosaccharidesVector.size(); i++)
    {
        uniqueMonosaccharidesPyList.append(uniqueMonosaccharidesVector[i]);
    }
    this->uniqueMonosaccharides = uniqueMonosaccharidesPyList;
    this->numberOfGlycosidicBonds = inputGlycan.obtain_total_number_of_glycosidic_bonds();
    this->glycosylationType = inputGlycan.get_type();
    
    std::string proteinResidue = inputGlycan.get_root().first.type().trim();
    std::string proteinResidueID = inputGlycan.get_root().first.id().trim();
    std::string proteinChainID = inputGlycan.get_chain().substr(0,1);
    this->chain_root_sugar_ID = inputGlycan.get_root_sugar_chainID();
    
    auto rootSummary = pybind11::dict("ProteinResidueType"_a=proteinResidue, "ProteinResidueID"_a=proteinResidueID, "ProteinChainID"_a=proteinChainID, "RootSugarChainID"_a=chain_root_sugar_ID);
    this->rootSummary = rootSummary;

    std::vector<float> torsions = inputGlycan.get_glycosylation_torsions();
    auto protein_glycan_linkage_torsion = pybind11::dict("Phi"_a=torsions[0], "Psi"_a=torsions[1]);
    this->protein_glycan_linkage_torsion = protein_glycan_linkage_torsion;

    std::vector<clipper::MSugar> list_of_sugars = inputGlycan.get_sugars();

    auto list = pybind11::list();
    for(int i = 0; i < list_of_sugars.size(); i++)
    {
        auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(inputGlycan, i, glycanID, parentGlycosylation, *this);
        list.append(sugarObject);
    }
    this->sugars = list;

    initialize_summary_of_glycan();
}

void privateer::pyanalysis::GlycanStructure::pyinitWithExperimentalData( const clipper::MGlycology& mgl, const int glycanID, privateer::pyanalysis::GlycosylationComposition& parentGlycosylationComposition, std::vector<std::pair< clipper::String , clipper::MSugar> >& finalLigandList)
{
    this->parentGlycosylation = parentGlycosylationComposition;
    std::vector<clipper::MGlycan> list_of_glycans = mgl.get_list_of_glycans();

    clipper::MGlycan inputGlycan = list_of_glycans[glycanID];

    this->glycan = inputGlycan;
    this->sugars_in_glycan = inputGlycan.get_sugars();
    
    this->glycanID = glycanID;
    this->numberOfSugars = inputGlycan.number_of_nodes();
    this->glycanWURCS = inputGlycan.generate_wurcs();

    auto uniqueMonosaccharidesPyList = pybind11::list();
    std::vector<std::string> uniqueMonosaccharidesVector = inputGlycan.obtain_unique_residue_codes();
    for(int i = 0; i < uniqueMonosaccharidesVector.size(); i++)
    {
        uniqueMonosaccharidesPyList.append(uniqueMonosaccharidesVector[i]);
    }
    this->uniqueMonosaccharides = uniqueMonosaccharidesPyList;
    this->numberOfGlycosidicBonds = inputGlycan.obtain_total_number_of_glycosidic_bonds();
    this->glycosylationType = inputGlycan.get_type();
    
    std::string proteinResidue = inputGlycan.get_root().first.type().trim();
    std::string proteinResidueID = inputGlycan.get_root().first.id().trim();
    std::string proteinChainID = inputGlycan.get_chain().substr(0,1);
    this->chain_root_sugar_ID = inputGlycan.get_root_sugar_chainID();
    
    auto rootSummary = pybind11::dict("ProteinResidueType"_a=proteinResidue, "ProteinResidueID"_a=proteinResidueID, "ProteinChainID"_a=proteinChainID, "RootSugarChainID"_a=chain_root_sugar_ID);
    this->rootSummary = rootSummary;

    std::vector<float> torsions = inputGlycan.get_glycosylation_torsions();
    auto protein_glycan_linkage_torsion = pybind11::dict("Phi"_a=torsions[0], "Psi"_a=torsions[1]);
    this->protein_glycan_linkage_torsion = protein_glycan_linkage_torsion;

    std::vector<clipper::MSugar> list_of_sugars_original = inputGlycan.get_sugars();
    std::vector<clipper::MSugar> list_of_sugars_modified;
    for(int i = 0; i < list_of_sugars_original.size(); i++)
    {
        for(int k = 0; k < finalLigandList.size(); k++)
        {
            if(list_of_sugars_original[i].size() == finalLigandList[k].second.size() && list_of_sugars_original[i].id().trim() == finalLigandList[k].second.id().trim() && list_of_sugars_original[i].type().trim() == finalLigandList[k].second.type().trim() && list_of_sugars_original[i].seqnum() == finalLigandList[k].second.seqnum())
                list_of_sugars_modified.push_back(finalLigandList[k].second);
        }
    }

    
    while(list_of_sugars_modified.size() > list_of_sugars_original.size())
    {
        for(int i = 0; i < list_of_sugars_modified.size(); i++)
        {
            clipper::String currentSugarPDBID = list_of_sugars_modified[i].id().trim();
            for(int k = i+1; k < list_of_sugars_modified.size(); k++)
            {
                if(currentSugarPDBID == list_of_sugars_modified[k].id().trim())
                    list_of_sugars_modified.erase(list_of_sugars_modified.begin() + k);
            }
        }
    }
    
    auto list = pybind11::list();
    for(int sugarID = 0; sugarID < list_of_sugars_modified.size(); sugarID++)
    {
        auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(inputGlycan, sugarID, glycanID, parentGlycosylation, *this, list_of_sugars_modified);
        list.append(sugarObject);
    }
    this->sugars = list;

    initialize_summary_of_glycan();
}

void privateer::pyanalysis::GlycanStructure::initialize_summary_of_glycan( )
{
    auto dict = pybind11::dict ("GlycanID"_a=glycanID, "WURCS"_a=glycanWURCS, "UniqueMonosaccharides"_a=uniqueMonosaccharides, "TotalSugars"_a=numberOfSugars, "NumberOfGlycosidicBonds"_a=numberOfGlycosidicBonds, "GlycosylationType"_a=glycosylationType, "RootInfo"_a=rootSummary, "ProteinGlycanLinkageTorsion"_a=protein_glycan_linkage_torsion, "ExperimentalData"_a=updatedWithExperimentalData);

    this->glycanSummary = dict;
}

privateer::pyanalysis::CarbohydrateStructure privateer::pyanalysis::GlycanStructure::get_monosaccharide(const int sugarID)
{
    if(sugarID >= numberOfSugars || sugarID < 0)
    {
        throw std::invalid_argument( "Provided ID is out of bounds and exceeds/inceeds number of sugars detected in the glycan. \nInput: " + std::to_string(sugarID) + "\tPermitted Range: [0-" + std::to_string(numberOfSugars - 1) + "]");
    }

    auto sugarObject = sugars[sugarID].cast<privateer::pyanalysis::CarbohydrateStructure>();
    return sugarObject;
}



pybind11::dict privateer::pyanalysis::GlycanStructure::query_glycomics_database( OfflineGlycomicsDatabase& importedDatabase, bool returnClosestMatches, bool returnAllPossiblePermutations, int nThreads )
{
    if(!glycoproteomicsDB.empty())
        return glycoproteomicsDB;
    else
    {
        std::vector<privateer::json::GlycomicsDatabase> glycomics_database = importedDatabase.return_imported_database();
        std::string currentWURCS = glycanWURCS;
        clipper::MGlycan currentGlycan = glycan;

        int valueLocation = privateer::util::find_index_of_value_from_wurcs(glycomics_database, glycanWURCS);
        if(!returnClosestMatches && currentGlycan.number_of_nodes() > 1)
        {
            if(valueLocation != -1 && glycomics_database[valueLocation].GlyConnectID != "NotFound")
            {
                std::string glytoucanID, glyconnectID;
                glytoucanID = glycomics_database[valueLocation].GlyTouCanID;
                if (glytoucanID.front() == '"' && glytoucanID.front() == '"')
                {
                    glytoucanID.erase(0, 1);
                    glytoucanID.pop_back();
                }
                glyconnectID = glycomics_database[valueLocation].GlyConnectID;
                if (glyconnectID.front() == '"' && glyconnectID.front() == '"')
                {
                    glyconnectID.erase(0, 1);
                    glyconnectID.pop_back();
                }

                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="Found a complete database match, no permutations were produced.", "glytoucan_id"_a=glytoucanID, "glyconnect_id"_a=glyconnectID );
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
            else if(valueLocation != -1 && glycomics_database[valueLocation].GlyConnectID == "NotFound")
            {
                std::string glytoucanID = glycomics_database[valueLocation].GlyTouCanID;
                if (glytoucanID.front() == '"' && glytoucanID.front() == '"')
                {
                    glytoucanID.erase(0, 1);
                    glytoucanID.pop_back();
                }

                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="Found a match for GlyTouCan ID, but GlyTouCan ID not found in GlyConnect. No permutations were generated due to False flag on returnClosestMatches", "glytoucan_id"_a=glytoucanID, "glyconnect_id"_a="Not Found" );
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
            else
            {
                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="GlyTouCan ID not found, no permutations were generated to find the closest match", "glytoucan_id"_a="Not Found", "glyconnect_id"_a="Not Found");
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
        }
        else if(returnClosestMatches && currentGlycan.number_of_nodes() > 1)
        {
            std::string glytoucanID = "Not Found";
            if(valueLocation != -1)
            {
                glytoucanID = glycomics_database[valueLocation].GlyTouCanID;
                if (glytoucanID.front() == '"' && glytoucanID.front() == '"')
                {
                    glytoucanID.erase(0, 1);
                    glytoucanID.pop_back();
                }
            }

            int detectedThreads = std::thread::hardware_concurrency();
            bool useParallelism = true;
            
            if(nThreads < 0)
                nThreads = detectedThreads;
            
            else if(nThreads < 2 && nThreads > -1)
            {
                useParallelism = false;
            }
            else if(nThreads > detectedThreads)
            {
                std::cout << "Error: More cores/threads were inputted as an argument, than detected on the system." 
                << "\n\tNumber of Available Cores/Threads detected on the system: " << detectedThreads 
                << "\n\tNumber of Cores/Threads requested via input argument: " << nThreads << "." << std::endl;

                throw std::invalid_argument( "Number of inputted threads exceed the number of detected threads." );
            }

            if(useParallelism)
            {
                std::cout << std::endl << "THREADING: Privateer will use " << nThreads << " of the " << detectedThreads << " available threads..." << std::endl;
            }

            std::vector<std::pair<clipper::MGlycan, std::vector<int>>> alternativeGlycans;
            
            if(useParallelism) alternativeGlycans = generate_closest_matches_parallel(currentGlycan, glycomics_database, returnAllPossiblePermutations, false, nThreads, useParallelism);
            else               alternativeGlycans = generate_closest_matches_singlethreaded(currentGlycan, glycomics_database, returnAllPossiblePermutations, false);   
            
            if(!alternativeGlycans.empty())
            {
                std::vector<std::pair<clipper::MGlycan, std::vector<int>>> uniqueAlternativeGlycans;
                for (int i = 0; i < alternativeGlycans.size(); i++)
                {
                    auto tempObject = alternativeGlycans[i];
                    clipper::MGlycan currentPermutatedGlycan = tempObject.first;
                    clipper::String currentPermutatedGlycanWURCS = currentPermutatedGlycan.generate_wurcs();
                    
                    if(uniqueAlternativeGlycans.empty())
                    {
                        uniqueAlternativeGlycans.push_back(tempObject);
                        continue;
                    }

                    for(int j = 0; j < uniqueAlternativeGlycans.size(); j++)
                    {
                        clipper::MGlycan currentPermutatedGlycanInFinalContainer = uniqueAlternativeGlycans[j].first;
                        clipper::String currentPermutatedGlycanWURCSInFinalContainer = currentPermutatedGlycanInFinalContainer.generate_wurcs();

                        if(currentPermutatedGlycanWURCS == currentPermutatedGlycanWURCSInFinalContainer)
                            break;

                        if(j == (uniqueAlternativeGlycans.size() - 1))
                            uniqueAlternativeGlycans.push_back(tempObject);
                    }
                }
                
                std::vector<std::pair<std::pair<clipper::MGlycan, std::vector<int>>,float>> finalGlycanPermutationContainer;
                for (int i = 0; i < uniqueAlternativeGlycans.size(); i++)
                {
                    int originalGlycanLength = currentGlycan.number_of_nodes(),
                        currentGlycanLength = uniqueAlternativeGlycans[i].first.number_of_nodes();

                    int anomerPermutations = uniqueAlternativeGlycans[i].second[0],
                        residuePermutations = uniqueAlternativeGlycans[i].second[1],
                        residueDeletions = uniqueAlternativeGlycans[i].second[2];
                    
                    float finalScore, maxPermutationScore, currentPermutationScore;

                    maxPermutationScore = ( ( (currentGlycanLength * 5) + (currentGlycanLength * 25) + ((originalGlycanLength - 1) * 100) ) / originalGlycanLength );
                    currentPermutationScore = ( ( (anomerPermutations * 5) + (residuePermutations * 25) + (residueDeletions * 100) ) / originalGlycanLength );
                    finalScore = (currentPermutationScore / maxPermutationScore) * 100;

                    auto tempObject = std::make_pair(uniqueAlternativeGlycans[i], finalScore);
                    finalGlycanPermutationContainer.push_back(tempObject);
                }
                
                // sort by permutation score for sensical output to the user.
                std::sort(finalGlycanPermutationContainer.begin(), finalGlycanPermutationContainer.end(), [](std::pair<std::pair<clipper::MGlycan, std::vector<int>>,float> a, std::pair<std::pair<clipper::MGlycan, std::vector<int>>,float> b){
                    return a.second < b.second;
                });

                
                this->outputGlycanPermutationContainer = finalGlycanPermutationContainer;
                auto permutationDatabaseOutputList = pybind11::list();
                for(int i = 0; i < finalGlycanPermutationContainer.size(); i++)
                {
                    clipper::String temporaryWURCS = finalGlycanPermutationContainer[i].first.first.generate_wurcs();
                    int valueLocation = privateer::util::find_index_of_value_from_wurcs(glycomics_database, temporaryWURCS);

                    std::string permutationGlyTouCanID = "Not Found";
                    permutationGlyTouCanID = glycomics_database[valueLocation].GlyConnectID;
                    if (permutationGlyTouCanID.front() == '"' && permutationGlyTouCanID.front() == '"')
                    {
                        permutationGlyTouCanID.erase(0, 1);
                        permutationGlyTouCanID.pop_back();
                    }                    
                    std::string permutationWURCS = temporaryWURCS;
                    auto permutationGlyConnectID = glycomics_database[valueLocation].GlyConnectID;
                    if (permutationGlyConnectID.front() == '"' && permutationGlyConnectID.front() == '"')
                    {
                        permutationGlyConnectID.erase(0, 1);
                        permutationGlyConnectID.pop_back();
                    }
                    float permutationScore = finalGlycanPermutationContainer[i].second;
                    int anomerPermutations = finalGlycanPermutationContainer[i].first.second[0];
                    int residuePermutations = finalGlycanPermutationContainer[i].first.second[1];
                    int residueDeletions = finalGlycanPermutationContainer[i].first.second[2];

                    auto permutationInfo = pybind11::dict ("wurcs"_a=permutationWURCS, "glytoucan_id"_a=permutationGlyTouCanID, "glyconnect_id"_a=permutationGlyConnectID, "permutation_score"_a=permutationScore, "anomer_permutations"_a=anomerPermutations, "residue_permutations"_a=residuePermutations, "residue_deletions"_a=residueDeletions);
                    permutationDatabaseOutputList.append(permutationInfo);
                }   
                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="Permutations were generated with closest matches detected", "glytoucan_id"_a=glytoucanID, "glyconnect_id"_a="Not Found", "permutations"_a=permutationDatabaseOutputList);
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
            else
            {
                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="Permutations were generated but no closest match detected", "glytoucan_id"_a=glytoucanID, "glyconnect_id"_a="Not Found");
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
        }
        else 
        {
            std::string glytoucanID = "Not Found";
            if(valueLocation != -1 && glycomics_database[valueLocation].GlyConnectID != "NotFound")
            {
                std::string glytoucanID, glyconnectID;
                glytoucanID = glycomics_database[valueLocation].GlyTouCanID;
                if (glytoucanID.front() == '"' && glytoucanID.front() == '"')
                {
                    glytoucanID.erase(0, 1);
                    glytoucanID.pop_back();
                }
                glyconnectID = glycomics_database[valueLocation].GlyConnectID;
                if (glyconnectID.front() == '"' && glyconnectID.front() == '"')
                {
                    glyconnectID.erase(0, 1);
                    glyconnectID.pop_back();
                }
                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="Found a complete database match, no permutations were produced.", "glytoucan_id"_a=glytoucanID, "glyconnect_id"_a=glyconnectID );
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
            else if(valueLocation != -1 && glycomics_database[valueLocation].GlyConnectID == "NotFound")
            {
                std::string glytoucanID = glycomics_database[valueLocation].GlyTouCanID;
                if (glytoucanID.front() == '"' && glytoucanID.front() == '"')
                {
                    glytoucanID.erase(0, 1);
                    glytoucanID.pop_back();
                }
                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="Found a match for GlyTouCan ID, but GlyTouCan ID not found in GlyConnect. No permutations were generated as the input Glycan is too short for permutations algorithm", "glytoucan_id"_a=glytoucanID, "glyconnect_id"_a="Not Found" );
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
            else
            {
                auto databaseOutput = pybind11::dict ("wurcs"_a=currentWURCS, "comment"_a="GlyTouCan ID not found, no permutations were generated to find the closest match as the input glycan is too short for permutations algorithm.", "glytoucan_id"_a=glytoucanID, "glyconnect_id"_a="Not Found");
                this->glycoproteomicsDB = databaseOutput;
                update_summary_of_glycan_after_dbquery();
                return databaseOutput;
            }
        }
    }
}

pybind11::list privateer::pyanalysis::GlycanStructure::get_torsions_summary(OfflineTorsionsDatabase& importedDatabase)
{
    std::vector<clipper::MGlycan::MGlycanTorsionSummary> torsion_summary_of_glycan = glycan.return_torsion_summary_within_glycan();
    std::vector<privateer::json::TorsionsDatabase> torsions_database = importedDatabase.return_imported_database();
    auto output = pybind11::list();
    
    for(int i = 0; i < torsion_summary_of_glycan.size(); i++)
    {
        clipper::MGlycan::MGlycanTorsionSummary current_torsion = torsion_summary_of_glycan[i];
       
        auto search_result = std::find_if(torsions_database.begin(), torsions_database.end(), [current_torsion](privateer::json::TorsionsDatabase& element)
        {
            return current_torsion.first_residue_name == element.first_residue && current_torsion.second_residue_name == element.second_residue && current_torsion.type == element.type;
        });

        if(search_result != std::end(torsions_database))
        {
            privateer::json::TorsionsDatabase& found_torsions_in_database = *search_result;
            std::vector<std::pair<float, float>> database_torsions = found_torsions_in_database.torsions;
           
            std::vector<std::pair<float, float>> current_glycan_torsions = current_torsion.torsions;
            std::vector<std::pair<clipper::MAtom, clipper::MAtom>> current_glycan_atoms = current_torsion.atoms;

            auto database_psi = pybind11::list();
            auto database_phi = pybind11::list();
            auto glycan_torsions = pybind11::list();
            
            for(int j = 0; j < database_torsions.size(); j++)
            {
                database_phi.append(database_torsions[j].first);
                database_psi.append(database_torsions[j].second);
            }

            for(int j = 0; j < current_glycan_torsions.size(); j++)
            {
                float currentPhi = current_glycan_torsions[j].first;
                float currentPsi = current_glycan_torsions[j].second;
                std::string current_bond_descr = current_glycan_atoms[j].first.id().trim() + "-" + current_glycan_atoms[j].second.id().trim();
                auto torsion_dict = pybind11::dict("Phi"_a=currentPhi, "Psi"_a=currentPsi, "glycan_bond"_a=current_bond_descr);
                glycan_torsions.append(torsion_dict);
            }
            
            std::string root_string = glycan.get_root_for_filename();
            auto current_pair_dict = pybind11::dict("first_residue"_a=std::string(current_torsion.first_residue_name), "second_residue"_a=std::string(current_torsion.second_residue_name), "root_descr"_a=root_string, "detected_torsions"_a=glycan_torsions, "database_phi"_a=database_phi, "database_psi"_a=database_psi);
            output.append(current_pair_dict);
        }
    }
    return output;
}

pybind11::dict privateer::pyanalysis::GlycanStructure::get_SNFG_strings(bool includeClosestMatches)
{
    clipper::MGlycan inputGlycan = glycan;

    clipper::String inputWURCSClipper = inputGlycan.generate_wurcs();
    std::string inputWURCS = inputWURCSClipper;

    privateer::glycanbuilderplot::Plot inputGlycanPlot(false, true, inputGlycan.get_root_by_name(), false, true);
    inputGlycanPlot.plot_glycan ( inputGlycan );
    std::string inputGlycanSNFG = inputGlycanPlot.write_to_string();

    if(includeClosestMatches && !outputGlycanPermutationContainer.empty())
    {
        std::vector<std::pair<std::pair<clipper::MGlycan, std::vector<int>>,float>> permutationVector = outputGlycanPermutationContainer;
        auto permutations = pybind11::list();

        for(int j = 0; j < permutationVector.size(); j++)
        {
            clipper::MGlycan permutationGlycan = permutationVector[j].first.first;
            clipper::String permutationWURCSClipper = permutationGlycan.generate_wurcs();
            std::string permutationWURCS = permutationWURCSClipper;

            privateer::glycanbuilderplot::Plot permutationGlycanPlot(false, true, inputGlycan.get_root_by_name(), false, true);
            permutationGlycanPlot.plot_glycan ( permutationGlycan );
            std::string permutationGlycanSNFG = permutationGlycanPlot.write_to_string();

            auto permutationDict = pybind11::dict("permutationWURCS"_a=permutationWURCS, "permutationSNFG"_a=permutationGlycanSNFG);
            permutations.append(permutationDict);
        }
        auto outputDict = pybind11::dict("WURCS"_a=inputWURCS, "SNFG"_a=inputGlycanSNFG, "ClosestMatches"_a=permutations);
        return outputDict;
    }
    else
    {
        auto outputDict = pybind11::dict("WURCS"_a=inputWURCS, "SNFG"_a=inputGlycanSNFG, "ClosestMatches"_a="Inclusion of SNFG representation of closest matches either was disabled/not generated or no permutations were generated as input Glycan was originally found on the databases");
        return outputDict;
    }
}

void privateer::pyanalysis::GlycanStructure::update_with_experimental_data(privateer::pyanalysis::XRayData& xray_data)
{
    this->updatedWithExperimentalData = true;
    std::vector<std::pair< clipper::String , clipper::MSugar> > finalLigandList = xray_data.get_finalLigandList();


    std::vector<clipper::MSugar> list_of_sugars_original = glycan.get_sugars();
    std::vector<clipper::MSugar> list_of_sugars_modified;
    for(int i = 0; i < list_of_sugars_original.size(); i++)
    {
        for(int k = 0; k < finalLigandList.size(); k++)
        {
            if(finalLigandList[k].first == glycan.get_chain().substr(0,1) && list_of_sugars_original[i].id().trim() == finalLigandList[k].second.id().trim() && list_of_sugars_original[i].short_name() == finalLigandList[k].second.short_name())
                list_of_sugars_modified.push_back(finalLigandList[k].second);
        }
    }

    
    while(list_of_sugars_modified.size() > list_of_sugars_original.size())
    {
        for(int i = 0; i < list_of_sugars_modified.size(); i++)
        {
            clipper::String currentSugarPDBID = list_of_sugars_modified[i].id().trim();
            for(int k = i+1; k < list_of_sugars_modified.size(); k++)
            {
                if(currentSugarPDBID == list_of_sugars_modified[k].id().trim())
                    list_of_sugars_modified.erase(list_of_sugars_modified.begin() + k);
            }
        }
    }
    
    auto list = pybind11::list();
    for(int sugarID = 0; sugarID < list_of_sugars_modified.size(); sugarID++)
    {
        auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(glycan, sugarID, glycanID, parentGlycosylation, *this, list_of_sugars_modified);
        list.append(sugarObject);
    }
    this->sugars = list;

    initialize_summary_of_glycan();
}

void privateer::pyanalysis::GlycanStructure::update_with_experimental_data(privateer::pyanalysis::CryoEMData& cryoem_data)
{
    this->updatedWithExperimentalData = true;
    std::vector<std::pair< clipper::String , clipper::MSugar> > finalLigandList = cryoem_data.get_finalLigandList();


    std::vector<clipper::MSugar> list_of_sugars_original = glycan.get_sugars();
    std::vector<clipper::MSugar> list_of_sugars_modified;
    for(int i = 0; i < list_of_sugars_original.size(); i++)
    {
        for(int k = 0; k < finalLigandList.size(); k++)
        {
            if(finalLigandList[k].first == glycan.get_chain().substr(0,1) && list_of_sugars_original[i].id().trim() == finalLigandList[k].second.id().trim() && list_of_sugars_original[i].short_name() == finalLigandList[k].second.short_name())
                list_of_sugars_modified.push_back(finalLigandList[k].second);
        }
    }

    
    while(list_of_sugars_modified.size() > list_of_sugars_original.size())
    {
        for(int i = 0; i < list_of_sugars_modified.size(); i++)
        {
            clipper::String currentSugarPDBID = list_of_sugars_modified[i].id().trim();
            for(int k = i+1; k < list_of_sugars_modified.size(); k++)
            {
                if(currentSugarPDBID == list_of_sugars_modified[k].id().trim())
                    list_of_sugars_modified.erase(list_of_sugars_modified.begin() + k);
            }
        }
    }
    
    auto list = pybind11::list();
    for(int sugarID = 0; sugarID < list_of_sugars_modified.size(); sugarID++)
    {
        auto sugarObject = privateer::pyanalysis::CarbohydrateStructure(glycan, sugarID, glycanID, parentGlycosylation, *this, list_of_sugars_modified);
        list.append(sugarObject);
    }
    this->sugars = list;

    initialize_summary_of_glycan();
}
///////////////////////////////////////////////// Class GlycanStructure END ////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////// Class CarbohydrateStructure  ////////////////////////////////////////////////////////////////////
void privateer::pyanalysis::CarbohydrateStructure::pyinit( clipper::MGlycan& mglycan, const int sugarID, const int glycanID, privateer::pyanalysis::GlycosylationComposition& parentGlycosylationComposition, privateer::pyanalysis::GlycanStructure& parentGlycanStructure)
{
    this->parentGlycosylation = parentGlycosylationComposition;
    this->parentGlycanStructure = parentGlycanStructure;
    
    std::vector<clipper::MSugar> list_of_sugars = mglycan.get_sugars();

    clipper::MSugar inputSugar = list_of_sugars[sugarID];
    
    this->parentGlycan = mglycan;
    this->sugar = inputSugar;
    this->sugarID = sugarID;
    this->glycanID = glycanID;
    this->sugar_conformation_code = inputSugar.conformation_code();
    this->sugar_conformation_name = clipper::data::conformational_landscape[sugar_conformation_code];
    this->sugar_conformation_name_iupac = clipper::data::iupac_conformational_landscape[sugar_conformation_code];
    this->sugar_puckering_amplitude = inputSugar.puckering_amplitude();
    this->sugar_anomer = inputSugar.anomer();
    this->sugar_handedness = inputSugar.handedness();
    this->sugar_denomination = inputSugar.type_of_sugar();
    this->sugar_ring_cardinality = inputSugar.ring_cardinality();

    std::vector<clipper::ftype> cremer_pople_params_vector = inputSugar.cremer_pople_params();
    auto sugar_cremer_pople_params = pybind11::list();
    for(int i = 0; i < cremer_pople_params_vector.size(); i++)
    {
        float currentParam = cremer_pople_params_vector[i];
        sugar_cremer_pople_params.append(currentParam);
    }
    this->sugar_cremer_pople_params=sugar_cremer_pople_params;

    this->sugar_sane = inputSugar.is_sane();

    std::string sugardiagnostic;
    if(inputSugar.is_sane())
    {
        if(!inputSugar.ok_with_conformation())
        {
            sugardiagnostic = "check";
        }
        else
            sugardiagnostic = "yes";
    }
    else
        sugardiagnostic = "no";

    this->privateer_diagnostic = sugardiagnostic;

    this->sugar_name_full = inputSugar.full_name();
    this->sugar_name_short = inputSugar.short_name();
    this->sugar_seqnum = inputSugar.get_seqnum();
    this->sugar_pdb_id = inputSugar.id();
    this->sugar_pdb_chain = mglycan.get_chain().substr(0,1);
    this->sugar_type = inputSugar.full_type();

    std::vector<clipper::ftype> sugar_ring_angles_vector = inputSugar.ring_angles();
    auto sugar_ring_angles = pybind11::list();
    for(int i = 0; i < sugar_ring_angles_vector.size(); i++)
    {
        float currentParam = sugar_ring_angles_vector[i];
        sugar_ring_angles.append(currentParam);
    }
    this->sugar_ring_angles=sugar_ring_angles;

    std::vector<clipper::ftype> sugar_ring_bonds_vector = inputSugar.ring_bonds();
    auto sugar_ring_bonds = pybind11::list();
    for(int i = 0; i < sugar_ring_bonds_vector.size(); i++)
    {
        float currentParam = sugar_ring_bonds_vector[i];
        sugar_ring_bonds.append(currentParam);
    }
    this->sugar_ring_bonds=sugar_ring_bonds;

    std::vector<clipper::ftype> sugar_ring_torsions_vector = inputSugar.ring_torsions();
    auto sugar_ring_torsion = pybind11::list();
    for(int i = 0; i < sugar_ring_torsions_vector.size(); i++)
    {
        float currentParam = sugar_ring_torsions_vector[i];
        sugar_ring_torsion.append(currentParam);
    }
    this->sugar_ring_torsion=sugar_ring_torsion;

    this->sugar_context=mglycan.get_type();
    this->sugar_ring_bond_rmsd=inputSugar.ring_bond_rmsd();
    this->sugar_ring_angle_rmsd=inputSugar.ring_angle_rmsd();
    this->sugar_bfactor=inputSugar.get_bfactor();
    this->sugar_supported=inputSugar.is_supported();
    this->sugar_diag_ring=inputSugar.ok_with_ring();
    this->sugar_diag_bonds_rmsd=inputSugar.ok_with_bonds_rmsd();
    this->sugar_diag_angles_rmsd=inputSugar.ok_with_angles_rmsd();
    this->sugar_diag_anomer=inputSugar.ok_with_anomer();
    this->sugar_diag_chirality=inputSugar.ok_with_chirality();
    this->sugar_diag_conformation=inputSugar.ok_with_conformation();
    this->sugar_diag_puckering=inputSugar.ok_with_puckering();

    if(sugar_context == "c-glycan")
        if ( inputSugar.type().trim() == "MAN" ) {
            if ( inputSugar.conformation_name() == "1c4" ) {
                inputSugar.override_conformation_diag ( true );
                this->sugar_diag_conformation=inputSugar.ok_with_conformation();
            }
        }

    this->sugarNode = parentGlycan.get_node(sugarID);
    this->sugar_connections=sugarNode.number_of_connections();

    auto ringatoms = pybind11::list();
    std::vector<clipper::MAtom> ringmembers = inputSugar.ring_members();
    for(int i = 0; i < ringmembers.size(); i++)
    {
        std::string atomname = ringmembers[i].id().trim();
        ringatoms.append(atomname);
    }
    this->sugar_ring_atoms = ringatoms;

    auto sugar_linkage_info_list_tmp = pybind11::list();
    if (sugarNode.number_of_connections() > 0)
    {
        for (int j = 0; j < sugarNode.number_of_connections(); j++ )
        {
            std::ostringstream linkagePositiontmp;
            int connectedToNodeID = sugarNode.get_connection(j).get_linked_node_id();
            linkagePositiontmp << sugarNode.get_connection(j).get_order();
            std::pair<clipper::MAtom, clipper::MAtom> linkage_atoms = sugarNode.get_connection(j).get_linkage_atoms();
            std::vector<float> linkage_torsions = sugarNode.get_connection(j).get_torsions();
            std::string donorAtomID = linkage_atoms.first.id().trim();
            std::string acceptorAtomID = linkage_atoms.second.id().trim();
            std::string donorPosition = linkagePositiontmp.str();
            std::string acceptorPosition;

            if (sugar.full_type() == "ketose")
                acceptorPosition += "2";
            else
                acceptorPosition += "1";

            auto linkage_torsion_dict = pybind11::dict();
            if (linkage_torsions.size() == 2)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1]);
            else if(linkage_torsions.size() == 3)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1], "omega"_a=linkage_torsions[2]);
            else if(linkage_torsions.size() == 4)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1], "omega_six"_a=linkage_torsions[2], "omega_seven"_a=linkage_torsions[3]);
            else if(linkage_torsions.size() == 6)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1], "omega_seven"_a=linkage_torsions[2], "omega_eight"_a=linkage_torsions[3], "omega_nine"_a=linkage_torsions[4], "phic1c2o8c8"_a=linkage_torsions[5]);

            auto sugar_linkage_info = pybind11::dict( "linkageID"_a=j, "connectedToSugarID"_a=connectedToNodeID, "donorPosition"_a=donorPosition, "acceptorPosition"_a=acceptorPosition, "donorAtom"_a=donorAtomID, "acceptorAtom"_a=acceptorAtomID, "linkageTorsions"_a=linkage_torsion_dict);
            sugar_linkage_info_list_tmp.append(sugar_linkage_info);
        }
    }

    this->sugar_linkages = sugar_linkage_info_list_tmp;


    initialize_summary_of_sugar();
}

void privateer::pyanalysis::CarbohydrateStructure::pyinitLigand( const int sugarID, std::vector<std::pair<clipper::String, clipper::MSugar>>& inputSugarList, privateer::pyanalysis::GlycosylationComposition& parentGlycosylationComposition )
{
    this->parentGlycosylation = parentGlycosylationComposition;
    
    std::pair<clipper::String, clipper::MSugar> inputSugar = inputSugarList[sugarID];
    this->parentGlycan = clipper::MGlycan();
    this->sugar = inputSugar.second;
    this->sugarID = sugarID;
    this->glycanID = -1;
    this->sugar_conformation_code = inputSugar.second.conformation_code();
    this->sugar_conformation_name = clipper::data::conformational_landscape[sugar_conformation_code];
    this->sugar_conformation_name_iupac = clipper::data::iupac_conformational_landscape[sugar_conformation_code];
    this->sugar_puckering_amplitude = inputSugar.second.puckering_amplitude();
    this->sugar_anomer = inputSugar.second.anomer();
    this->sugar_handedness = inputSugar.second.handedness();
    this->sugar_denomination = inputSugar.second.type_of_sugar();
    this->sugar_ring_cardinality = inputSugar.second.ring_cardinality();

    std::vector<clipper::ftype> cremer_pople_params_vector = inputSugar.second.cremer_pople_params();
    auto sugar_cremer_pople_params = pybind11::list();
    for(int i = 0; i < cremer_pople_params_vector.size(); i++)
    {
        float currentParam = cremer_pople_params_vector[i];
        sugar_cremer_pople_params.append(currentParam);
    }
    this->sugar_cremer_pople_params=sugar_cremer_pople_params;
    this->sugar_sane = inputSugar.second.is_sane();

    std::string sugardiagnostic;
    if(inputSugar.second.is_sane())
    {
        if(!inputSugar.second.ok_with_conformation())
        {
            sugardiagnostic = "check";
        }
        else
            sugardiagnostic = "yes";
    }
    else
        sugardiagnostic = "no";

    this->privateer_diagnostic = sugardiagnostic;

    this->sugar_name_full = inputSugar.second.full_name();
    this->sugar_name_short = inputSugar.second.short_name();
    this->sugar_seqnum = inputSugar.second.get_seqnum();
    this->sugar_pdb_id = inputSugar.second.id();
    this->sugar_pdb_chain = inputSugar.first;
    this->sugar_type = inputSugar.second.full_type();

    std::vector<clipper::ftype> sugar_ring_angles_vector = inputSugar.second.ring_angles();
    auto sugar_ring_angles = pybind11::list();
    for(int i = 0; i < sugar_ring_angles_vector.size(); i++)
    {
        float currentParam = sugar_ring_angles_vector[i];
        sugar_ring_angles.append(currentParam);
    }
    this->sugar_ring_angles=sugar_ring_angles;

    std::vector<clipper::ftype> sugar_ring_bonds_vector = inputSugar.second.ring_bonds();
    auto sugar_ring_bonds = pybind11::list();
    for(int i = 0; i < sugar_ring_bonds_vector.size(); i++)
    {
        float currentParam = sugar_ring_bonds_vector[i];
        sugar_ring_bonds.append(currentParam);
    }
    this->sugar_ring_bonds=sugar_ring_bonds;

    std::vector<clipper::ftype> sugar_ring_torsions_vector = inputSugar.second.ring_torsions();
    auto sugar_ring_torsion = pybind11::list();
    for(int i = 0; i < sugar_ring_torsions_vector.size(); i++)
    {
        float currentParam = sugar_ring_torsions_vector[i];
        sugar_ring_torsion.append(currentParam);
    }
    this->sugar_ring_torsion=sugar_ring_torsion;

    this->sugar_context=inputSugar.second.get_context();
    this->sugar_ring_bond_rmsd=inputSugar.second.ring_bond_rmsd();
    this->sugar_ring_angle_rmsd=inputSugar.second.ring_angle_rmsd();
    this->sugar_bfactor=inputSugar.second.get_bfactor();
    this->sugar_supported=inputSugar.second.is_supported();
    this->sugar_diag_ring=inputSugar.second.ok_with_ring();
    this->sugar_diag_bonds_rmsd=inputSugar.second.ok_with_bonds_rmsd();
    this->sugar_diag_angles_rmsd=inputSugar.second.ok_with_angles_rmsd();
    this->sugar_diag_anomer=inputSugar.second.ok_with_anomer();
    this->sugar_diag_chirality=inputSugar.second.ok_with_chirality();
    this->sugar_diag_conformation=inputSugar.second.ok_with_conformation();
    this->sugar_diag_puckering=inputSugar.second.ok_with_puckering();

    auto ringatoms = pybind11::list();
    std::vector<clipper::MAtom> ringmembers = inputSugar.second.ring_members();
    for(int i = 0; i < ringmembers.size(); i++)
    {
        std::string atomname = ringmembers[i].id().trim();
        ringatoms.append(atomname);
    }
    this->sugar_ring_atoms = ringatoms;

    if(sugar_context == "c-glycan")
        if ( inputSugar.second.type().trim() == "MAN" ) {
            if ( inputSugar.second.conformation_name() == "1c4" ) {
                inputSugar.second.override_conformation_diag ( true );
                this->sugar_diag_conformation=inputSugar.second.ok_with_conformation();
            }
        }

    this->sugar_rscc=inputSugar.second.get_rscc();
    this->sugar_accum=inputSugar.second.get_accum();
    this->sugar_occupancy_check=inputSugar.second.get_occupancy_check();
    

    this->sugarNode = clipper::MGlycan::Node(); // we don't have an MGlycan object with free float ligands...
    this->sugar_connections = 0;
    auto sugar_linkage_info_list_tmp = pybind11::list();


    this->sugar_linkages = sugar_linkage_info_list_tmp;
    initialize_summary_of_sugar();
}

void privateer::pyanalysis::CarbohydrateStructure::pyinitWithExperimentalData( clipper::MGlycan& mglycan, const int sugarID, const int glycanID, privateer::pyanalysis::GlycosylationComposition& parentGlycosylationComposition, privateer::pyanalysis::GlycanStructure& parentGlycanStructure, std::vector<clipper::MSugar>& list_of_sugars)
{
    this->parentGlycosylation = parentGlycosylationComposition;
    this->parentGlycanStructure = parentGlycanStructure;

    clipper::MSugar inputSugar = list_of_sugars[sugarID];
    
    this->parentGlycan = mglycan;
    this->sugar = inputSugar;
    this->sugarID = sugarID;
    this->glycanID = glycanID;
    this->sugar_conformation_code = inputSugar.conformation_code();
    this->sugar_conformation_name = clipper::data::conformational_landscape[sugar_conformation_code];
    this->sugar_conformation_name_iupac = clipper::data::iupac_conformational_landscape[sugar_conformation_code];
    this->sugar_puckering_amplitude = inputSugar.puckering_amplitude();
    this->sugar_anomer = inputSugar.anomer();
    this->sugar_handedness = inputSugar.handedness();
    this->sugar_denomination = inputSugar.type_of_sugar();
    this->sugar_ring_cardinality = inputSugar.ring_cardinality();

    std::vector<clipper::ftype> cremer_pople_params_vector = inputSugar.cremer_pople_params();
    auto sugar_cremer_pople_params = pybind11::list();
    for(int i = 0; i < cremer_pople_params_vector.size(); i++)
    {
        float currentParam = cremer_pople_params_vector[i];
        sugar_cremer_pople_params.append(currentParam);
    }
    this->sugar_cremer_pople_params=sugar_cremer_pople_params;

    this->sugar_sane = inputSugar.is_sane();

    std::string sugardiagnostic;
    if(inputSugar.is_sane())
    {
        if(!inputSugar.ok_with_conformation())
        {
            sugardiagnostic = "check";
        }
        else
            sugardiagnostic = "yes";
    }
    else
        sugardiagnostic = "no";

    this->privateer_diagnostic = sugardiagnostic;

    this->sugar_name_full = inputSugar.full_name();
    this->sugar_name_short = inputSugar.short_name();
    this->sugar_seqnum = inputSugar.get_seqnum();
    this->sugar_pdb_id = inputSugar.id();
    this->sugar_pdb_chain = mglycan.get_chain().substr(0,1);
    this->sugar_type = inputSugar.full_type();

    std::vector<clipper::ftype> sugar_ring_angles_vector = inputSugar.ring_angles();
    auto sugar_ring_angles = pybind11::list();
    for(int i = 0; i < sugar_ring_angles_vector.size(); i++)
    {
        float currentParam = sugar_ring_angles_vector[i];
        sugar_ring_angles.append(currentParam);
    }
    this->sugar_ring_angles=sugar_ring_angles;

    std::vector<clipper::ftype> sugar_ring_bonds_vector = inputSugar.ring_bonds();
    auto sugar_ring_bonds = pybind11::list();
    for(int i = 0; i < sugar_ring_bonds_vector.size(); i++)
    {
        float currentParam = sugar_ring_bonds_vector[i];
        sugar_ring_bonds.append(currentParam);
    }
    this->sugar_ring_bonds=sugar_ring_bonds;

    std::vector<clipper::ftype> sugar_ring_torsions_vector = inputSugar.ring_torsions();
    auto sugar_ring_torsion = pybind11::list();
    for(int i = 0; i < sugar_ring_torsions_vector.size(); i++)
    {
        float currentParam = sugar_ring_torsions_vector[i];
        sugar_ring_torsion.append(currentParam);
    }
    this->sugar_ring_torsion=sugar_ring_torsion;

    this->sugar_ring_bond_rmsd=inputSugar.ring_bond_rmsd();
    this->sugar_ring_angle_rmsd=inputSugar.ring_angle_rmsd();
    this->sugar_bfactor=inputSugar.get_bfactor();
    this->sugar_supported=inputSugar.is_supported();
    this->sugar_diag_ring=inputSugar.ok_with_ring();
    this->sugar_diag_bonds_rmsd=inputSugar.ok_with_bonds_rmsd();
    this->sugar_diag_angles_rmsd=inputSugar.ok_with_angles_rmsd();
    this->sugar_diag_anomer=inputSugar.ok_with_anomer();
    this->sugar_diag_chirality=inputSugar.ok_with_chirality();
    this->sugar_diag_conformation=inputSugar.ok_with_conformation();
    this->sugar_diag_puckering=inputSugar.ok_with_puckering();
    this->sugar_rscc=inputSugar.get_rscc();
    this->sugar_accum=inputSugar.get_accum();
    this->sugar_occupancy_check=inputSugar.get_occupancy_check();
    this->sugar_context=mglycan.get_type();

    auto ringatoms = pybind11::list();
    std::vector<clipper::MAtom> ringmembers = inputSugar.ring_members();
    for(int i = 0; i < ringmembers.size(); i++)
    {
        std::string atomname = ringmembers[i].id().trim();
        ringatoms.append(atomname);
    }
    this->sugar_ring_atoms = ringatoms;

    this->sugarNode = parentGlycan.get_node(sugarID);
    this->sugar_connections=sugarNode.number_of_connections();

    auto sugar_linkage_info_list_tmp = pybind11::list();
    if (sugarNode.number_of_connections() > 0)
    {
        for (int j = 0; j < sugarNode.number_of_connections(); j++ )
        {
            std::ostringstream linkagePositiontmp;
            int connectedToNodeID = sugarNode.get_connection(j).get_linked_node_id();
            linkagePositiontmp << sugarNode.get_connection(j).get_order();
            std::pair<clipper::MAtom, clipper::MAtom> linkage_atoms = sugarNode.get_connection(j).get_linkage_atoms();
            std::vector<float> linkage_torsions = sugarNode.get_connection(j).get_torsions();
            std::string donorAtomID = linkage_atoms.first.id().trim();
            std::string acceptorAtomID = linkage_atoms.second.id().trim();
            std::string donorPosition = linkagePositiontmp.str();
            std::string acceptorPosition;

            if (sugar.full_type() == "ketose")
                acceptorPosition += "2";
            else
                acceptorPosition += "1";

            auto linkage_torsion_dict = pybind11::dict();
            if (linkage_torsions.size() == 2)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1]);
            else if(linkage_torsions.size() == 3)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1], "omega"_a=linkage_torsions[2]);
            else if(linkage_torsions.size() == 4)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1], "omega_six"_a=linkage_torsions[2], "omega_seven"_a=linkage_torsions[3]);
            else if(linkage_torsions.size() == 6)
                linkage_torsion_dict = pybind11::dict("phi"_a=linkage_torsions[0], "psi"_a=linkage_torsions[1], "omega_seven"_a=linkage_torsions[2], "omega_eight"_a=linkage_torsions[3], "omega_nine"_a=linkage_torsions[4], "phic1c2o8c8"_a=linkage_torsions[5]);

            auto sugar_linkage_info = pybind11::dict( "linkageID"_a=j, "connectedToSugarID"_a=connectedToNodeID, "donorPosition"_a=donorPosition, "acceptorPosition"_a=acceptorPosition, "donorAtom"_a=donorAtomID, "acceptorAtom"_a=acceptorAtomID, "linkageTorsions"_a=linkage_torsion_dict);
            sugar_linkage_info_list_tmp.append(sugar_linkage_info);
        }
    }

    this->sugar_linkages = sugar_linkage_info_list_tmp;
    initialize_summary_of_sugar();
}


void privateer::pyanalysis::CarbohydrateStructure::initialize_summary_of_sugar( )
{
    auto dict = pybind11::dict ("sugarID"_a=sugarID, "glycanID"_a=glycanID, "sugar_pdb_id"_a=sugar_pdb_id, "sugar_pdb_chain"_a=sugar_pdb_chain, "sugar_seqnum"_a=sugar_seqnum, "sugar_name_full"_a=sugar_name_full, "sugar_name_short"_a=sugar_name_short, "sugar_anomer"_a=sugar_anomer, "is_sane"_a=sugar_sane, "ExperimentalData"_a=updatedWithExperimentalData, "RSCC"_a=sugar_rscc, "mFo"_a=sugar_accum, "OccupancyCheck"_a=sugar_occupancy_check, "SugarLinkages"_a=sugar_linkages);

    this->sugarSummary = dict;
}

///////////////////////////////////////////////// Class CarbohydrateStructure END ////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Class XRayData ////////////////////////////////////////////////////////////////////

void privateer::pyanalysis::XRayData::read_from_file( std::string& path_to_mtz_file, std::string& path_to_model_file, std::string& input_column_fobs_user, float ipradius, int nThreads, bool debug_output) 
{
    bool rscc_best = false;
    this->debug_output = debug_output;

    bool useSigmaa = false;
    int detectedThreads = std::thread::hardware_concurrency();
    bool useParallelism = true;
    bool showGeom = false;

    
    if(nThreads < 0)
        nThreads = detectedThreads;
    else if(nThreads < 2 && nThreads > -1)
    {
        useParallelism = false;
    }
    else if(nThreads > detectedThreads)
    {
        std::cout << "Error: More cores/threads were inputted as an argument, than detected on the system." 
        << "\n\tNumber of Available Cores/Threads detected on the system: " << detectedThreads 
        << "\n\tNumber of Cores/Threads requested via input argument: " << nThreads << "." << std::endl;

        throw std::invalid_argument( "Number of inputted threads exceed the number of detected threads." );
    }

    if(useParallelism)
    {
        std::cout << std::endl << "THREADING: Privateer will use " << nThreads << " of the " << std::thread::hardware_concurrency() << " available threads..." << std::endl;
    }

    if(path_to_model_file == "undefined" || path_to_model_file == "")
    {
        throw std::invalid_argument( "No path was provided for model file input! Aborting." );
    }

    if(path_to_mtz_file == "undefined" || path_to_mtz_file == "")
    {
        throw std::invalid_argument( "No path was provided for .mtz file input! Aborting." );
    }

    clipper::String input_column_fobs_clipper = input_column_fobs_user;
    this->input_column_fobs = input_column_fobs_clipper;
    clipper::String path_to_model_file_clipper = path_to_model_file;
    this->path_to_model_file=path_to_model_file;
    clipper::String path_to_mtz_file_clipper = path_to_mtz_file;
    clipper::MMDBfile mfile;
    clipper::CCP4MTZfile mtzin;
    clipper::CCP4MTZfile ampmtzin;
    clipper::MTZcrystal opxtal;
    clipper::MTZdataset opdset;
    clipper::HKL_info hklinfo;

    privateer::util::read_coordinate_file_mtz(mfile, mmol, path_to_model_file_clipper, true);
    int pos_slash = path_to_model_file.rfind("/");
    privateer::xray::read_xray_map( path_to_mtz_file_clipper, path_to_model_file_clipper, mmol, hklinfo, mtzin );

    clipper::HKL_data<clipper::data32::F_sigF> fobs;            // allocate space for F and sigF
    clipper::HKL_data<clipper::data32::F_sigF> fobs_scaled;     // allocate space for scaled F and sigF
    clipper::HKL_data<clipper::data32::F_phi> fc_omit_bsc;      // allocate space for the omit calc data with bsc
    clipper::HKL_data<clipper::data32::F_phi> fc_all_bsc;       // allocate space for the whole calculated data with bsc
    clipper::HKL_data<clipper::data32::F_phi> fc_ligands_bsc;    // allocate space for the ligand calculated data

    fobs = clipper::HKL_data<clipper::data32::F_sigF> ( hklinfo );
    fobs_scaled = clipper::HKL_data<clipper::data32::F_sigF> ( hklinfo );
    fc_omit_bsc = clipper::HKL_data<clipper::data32::F_phi> ( hklinfo );
    fc_all_bsc = clipper::HKL_data<clipper::data32::F_phi> ( hklinfo );
    fc_ligands_bsc = clipper::HKL_data<clipper::data32::F_phi> ( hklinfo );
    privateer::xray::initialize_experimental_dataset( mtzin, ampmtzin, input_column_fobs, fobs, hklinfo, opxtal, opdset, path_to_mtz_file_clipper);

    std::cout << std::endl << " " << fobs.num_obs() << " reflections have been loaded";
        std::cout << std::endl << std::endl << " Resolution " << hklinfo.resolution().limit() << "Å" << std::endl << hklinfo.cell().format() << std::endl;

    this->hklinfo = hklinfo;
    fobs_scaled = fobs;

    std::vector<std::pair<clipper::String, clipper::MSugar>> ligandsOnly;
    clipper::Atom_list mainAtoms;
    clipper::Atom_list ligandAtoms;
    clipper::Atom_list allAtoms;

    std::vector<std::pair< clipper::String , clipper::MSugar> > ligandList; // we store the Chain ID and create an MSugar to be scored
    std::vector<clipper::MMonomer> sugarList; // store the original MMonomer

    const clipper::MAtomNonBond& manb = clipper::MAtomNonBond( mmol, 1.0 ); // was 1.0

    clipper::MGlycology mgl = clipper::MGlycology(mmol, manb, debug_output, "undefined");

    for ( int p = 0; p < mmol.size(); p++ )
    {
        for ( int m = 0; m < mmol[p].size(); m++ )
        {
            if ( clipper::MDisaccharide::search_disaccharides(mmol[p][m].type().c_str()) != -1 ) // treat disaccharide
            {
                clipper::MDisaccharide md(mmol, manb, mmol[p][m], debug_output );
                sugarList.push_back ( mmol[p][m] );
                sugarList.push_back ( mmol[p][m] );
                clipper::String id = mmol[p].id();
                id.resize(1);

                ligandList.push_back ( std::pair < clipper::String, clipper::MSugar> (id, md.get_first_sugar()));
                ligandList.push_back ( std::pair < clipper::String, clipper::MSugar> (id, md.get_second_sugar()));

                if (( md.get_first_sugar().type_of_sugar() == "unsupported" ) || ( md.get_second_sugar().type_of_sugar() == "unsupported" ) )
                {
                    throw std::invalid_argument( "Error: strangely, at least one of the sugars in the supplied PDB file is missing required atoms. Stopping..." );
                }

                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    ligandAtoms.push_back(mmol[p][m][id]);  // add the ligand atoms to a second array
                    allAtoms.push_back(mmol[p][m][id]);
                }

            }
            else if ( !clipper::MSugar::search_database(mmol[p][m].type().c_str()) )
            {
                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    mainAtoms.push_back(mmol[p][m][id]); // cycle through atoms and copy them
                    allAtoms.push_back(mmol[p][m][id]);
                }
            }
            else // it's one of the sugars contained in the database
            {
                clipper::MSugar msug, msug_b;

                std::vector <char> conformers = privateer::util::number_of_conformers(mmol[p][m]);

                // if(debug_output)
                // {
                //     std::cout << "number of alternate conformations: " << conformers.size() << std::endl;
                // }

                int n_conf = conformers.size();

                if ( n_conf > 0 )
                {
                    if ( n_conf == 1 )
                        msug   = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[0]);
                    else
                    {
                        msug   = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[0]);
                        msug_b = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[1]);
                    }

                }
                else
                {
                    msug = clipper::MSugar(mmol, mmol[p][m], manb, debug_output);
                }

                sugarList.push_back(mmol[p][m]);
                clipper::String id = mmol[p].id();
                id.resize(1);

                ligandList.push_back(std::pair<clipper::String, clipper::MSugar> (id, msug));
                // add both conformers if the current monomer contains more than one
                if ( n_conf == 2 )
                {
                    ligandList.push_back(std::pair<clipper::String, clipper::MSugar> (id, msug_b));
                    sugarList.push_back(mmol[p][m]);
                }

                if ( msug.type_of_sugar() == "unsupported" )
                {
                    throw std::invalid_argument( "Error: strangely, at least one of the sugars in the supplied PDB file is missing required atoms. Stopping..." );
                }

                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    ligandAtoms.push_back(mmol[p][m][id]);  // add the ligand atoms to a second array
                    allAtoms.push_back(mmol[p][m][id]);
                }
            }
        }
    }

    clipper::SFcalc_obs_bulk<float> sfcbligands;
    clipper::SFcalc_obs_bulk<float> sfcb;
    clipper::SFcalc_obs_bulk<float> sfcball;

    try
    {   // calculate structure factors with bulk solvent correction
        if(useParallelism && nThreads >= 2)
        {
            std::vector<std::future<void>> thread_results;
            thread_results.push_back(std::async(std::launch::async, 
                [](clipper::SFcalc_obs_bulk<float>& sfcbligands, clipper::HKL_data<clipper::data32::F_phi>& fc_ligands_bsc, clipper::HKL_data<clipper::data32::F_sigF>& fobs, clipper::Atom_list& ligandAtoms)
                {
                    sfcbligands( fc_ligands_bsc, fobs, ligandAtoms ); // was fobs_scaled
                },
                std::ref(sfcbligands), std::ref(fc_ligands_bsc), std::ref(fobs), std::ref(ligandAtoms)
                ));

            thread_results.push_back(std::async(std::launch::async, 
                [](clipper::SFcalc_obs_bulk<float>& sfcb, clipper::HKL_data<clipper::data32::F_phi>& fc_omit_bsc, clipper::HKL_data<clipper::data32::F_sigF>& fobs, clipper::Atom_list& mainAtoms)
                {
                    sfcb( fc_omit_bsc, fobs, mainAtoms ); // was fobs_scaled
                },
                std::ref(sfcb), std::ref(fc_omit_bsc), std::ref(fobs), std::ref(mainAtoms)
                ));

            thread_results.push_back(std::async(std::launch::async, 
                [](clipper::SFcalc_obs_bulk<float>& sfcball, clipper::HKL_data<clipper::data32::F_phi>& fc_all_bsc, clipper::HKL_data<clipper::data32::F_sigF>& fobs, clipper::Atom_list& allAtoms)
                {
                    sfcball( fc_all_bsc, fobs, allAtoms ); // was fobs_scaled
                },
                std::ref(sfcball), std::ref(fc_all_bsc), std::ref(fobs), std::ref(allAtoms)
                ));
            
            for (auto& r: thread_results)
                r.get();

            thread_results.clear();
        }
        else
        {
            sfcbligands( fc_ligands_bsc, fobs, ligandAtoms ); // was fobs_scaled

            sfcb( fc_omit_bsc, fobs, mainAtoms );  // calculation of omit SF with bulk solvent correction

            sfcball( fc_all_bsc, fobs, allAtoms ); // calculation of SF with bulk solvent correction
        }
    }
    catch ( ... )
    {
        std::cout << "\nThe input file has unrecognised atoms. Might cause unexpected results...\n";  // this causes clipper to freak out, so better remove those unknowns
    }


    fc_ligands_bsc[0].set_null();
    fc_omit_bsc[0].set_null();
    fc_all_bsc[0].set_null();


    clipper::Grid_sampling mygrid( hklinfo.spacegroup(), hklinfo.cell(), hklinfo.resolution() );  // define grid
    clipper::Xmap<float> sigmaa_all_map( hklinfo.spacegroup(), hklinfo.cell(), mygrid );          // define sigmaa best map
    clipper::Xmap<float> sigmaa_dif_map( hklinfo.spacegroup(), hklinfo.cell(), mygrid );          // define sigmaa diff  map
    clipper::Xmap<float> sigmaa_omit_fb( hklinfo.spacegroup(), hklinfo.cell(), mygrid );
    clipper::Xmap<float> sigmaa_omit_fd( hklinfo.spacegroup(), hklinfo.cell(), mygrid );          // define sigmaa omit diff map
    clipper::Xmap<float> ligandmap( hklinfo.spacegroup(), hklinfo.cell(), mygrid );

    // scale data and flag R-free

    HRI ih;
    clipper::HKL_data<clipper::data32::Flag> flag( hklinfo );     // same flag for both calculations, omit absent reflections
    clipper::SFscale_aniso<float> sfscale;


    if(useParallelism && nThreads >= 2)
    {
        std::vector<std::future<void>> thread_results;
        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::SFscale_aniso<float>& sfscale, clipper::HKL_data<clipper::data32::F_sigF>& fobs_scaled, clipper::HKL_data<clipper::data32::F_phi>& fc_all_bsc)
            {
                sfscale( fobs_scaled, fc_all_bsc ); // was fobs_scaled
            },
            std::ref(sfscale), std::ref(fobs_scaled), std::ref(fc_all_bsc)
            ));
        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::HKL_data<clipper::datatypes::Flag>& flag, clipper::HKL_data<clipper::data32::F_sigF>& fobs_scaled, HRI& ih)
            {
                for ( ih = flag.first(); !ih.last(); ih.next() ) // we want to use all available reflections
                {
                    if ( !fobs_scaled[ih].missing() ) flag[ih].flag() = clipper::SFweight_spline<float>::BOTH;
                    else flag[ih].flag() = clipper::SFweight_spline<float>::NONE;

                }
            },
            std::ref(flag), std::ref(fobs_scaled), std::ref(ih)
            ));
        
        
        for (auto& r: thread_results)
            r.get();

        thread_results.clear();
    }
    else
    {
        sfscale( fobs_scaled, fc_all_bsc );  // anisotropic scaling of Fobs. We scale Fobs to Fcalc instead of scaling our 3 Fcalcs to Fobs
        for ( ih = flag.first(); !ih.last(); ih.next() ) // we want to use all available reflections
        {
            if ( !fobs_scaled[ih].missing() ) flag[ih].flag() = clipper::SFweight_spline<float>::BOTH;
            else flag[ih].flag() = clipper::SFweight_spline<float>::NONE;
        }
    }

    int n_refln = 1000;
    int n_param = 20;
    double FobsFcalcSum = 0.0;
    double FobsFcalcAllSum = 0.0;
    double FobsSum = 0.0;

    clipper::HKL_data<clipper::data32::F_phi> fb_omit( hklinfo ); // new variables for omit sigmaa weighting calculation
    clipper::HKL_data<clipper::data32::F_phi> fd_omit( hklinfo );
    clipper::HKL_data<clipper::data32::Phi_fom> phiw_omit( hklinfo );
    clipper::HKL_data<clipper::data32::F_phi> fb_all( hklinfo ); // variables for all atom sigmaa weighting calculation
    clipper::HKL_data<clipper::data32::F_phi> fd_all( hklinfo );
    clipper::HKL_data<clipper::data32::Phi_fom> phiw_all( hklinfo );


    if(useParallelism && nThreads >= 2)
    {
        std::vector<std::future<void>> thread_results;
        thread_results.push_back(std::async(std::launch::async, 
            [](int n_refln, int n_param, clipper::HKL_data<clipper::data32::F_phi>& fb_omit, clipper::HKL_data<clipper::data32::F_phi>& fd_omit, 
            clipper::HKL_data<clipper::data32::Phi_fom>& phiw_omit, clipper::HKL_data<clipper::data32::F_sigF>& fobs_scaled, clipper::HKL_data<clipper::data32::F_phi>& fc_omit_bsc,
            clipper::HKL_data<clipper::datatypes::Flag>& flag)
            {
                clipper::SFweight_spline<float> sfw_omit (n_refln, n_param );
                sfw_omit( fb_omit, fd_omit, phiw_omit, fobs_scaled, fc_omit_bsc, flag );
            },
            n_refln, n_param, std::ref(fb_omit), std::ref(fd_omit), std::ref(phiw_omit), std::ref(fobs_scaled), std::ref(fc_omit_bsc), std::ref(flag)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](int n_refln, int n_param, clipper::HKL_data<clipper::data32::F_phi>& fb_all, clipper::HKL_data<clipper::data32::F_phi>& fd_all, 
            clipper::HKL_data<clipper::data32::Phi_fom>& phiw_all, clipper::HKL_data<clipper::data32::F_sigF>& fobs_scaled, clipper::HKL_data<clipper::data32::F_phi>& fc_all_bsc,
            clipper::HKL_data<clipper::datatypes::Flag>& flag)
            {
                clipper::SFweight_spline<float> sfw_all( n_refln, n_param );
                sfw_all( fb_all, fd_all, phiw_all, fobs_scaled, fc_all_bsc, flag );
            },
            n_refln, n_param, std::ref(fb_all), std::ref(fd_all), std::ref(phiw_all), std::ref(fobs_scaled), std::ref(fc_all_bsc), std::ref(flag)
            ));
        
        for (auto& r: thread_results)
            r.get();
        

        thread_results.clear();
    }
    else
    {
        clipper::SFweight_spline<float> sfw_omit (n_refln, n_param );
        sfw_omit( fb_omit, fd_omit, phiw_omit, fobs_scaled, fc_omit_bsc, flag );

        clipper::SFweight_spline<float> sfw_all( n_refln, n_param );
        sfw_all( fb_all,  fd_all,  phiw_all, fobs_scaled, fc_all_bsc, flag );
    }
    
    // fb:          output best map coefficients
    // fd:          output difference map coefficients
    // phiw:        output phase and fom
    // fobs_scaled: input observed structure factors, previously scaled
    // fc_omit_bsc: input calculated omit data, with bulk solvent correction
    // fc_all_bsc:  input calculated data, with bsc


    std::vector<double> params( n_param, 2.0 );
    clipper::BasisFn_spline wrk_basis( hklinfo, n_param, 2.0 );

    clipper::TargetFn_scaleF1F2<clipper::data32::F_phi,clipper::data32::F_sigF> wrk_target_omit( fc_omit_bsc, fobs_scaled ); // was just fobs
    clipper::TargetFn_scaleF1F2<clipper::data32::F_phi,clipper::data32::F_sigF> wrk_target_all ( fc_all_bsc, fobs_scaled );
    clipper::ResolutionFn wrk_scale_omit( hklinfo, wrk_basis, wrk_target_omit, params );
    clipper::ResolutionFn wrk_scale_all ( hklinfo, wrk_basis, wrk_target_all,  params );

    double Fo, Fc_all, Fc_omit;

    if(useParallelism && nThreads >= 2)
    {
        std::vector<std::future<void>> thread_results;
        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<float>& sigmaa_all_map, clipper::HKL_data<clipper::data32::F_phi>& fb_all)
            {
                sigmaa_all_map.fft_from( fb_all );
            },
            std::ref(sigmaa_all_map), std::ref(fb_all)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<float>& sigmaa_dif_map, clipper::HKL_data<clipper::data32::F_phi>& fd_all)
            {
                sigmaa_dif_map.fft_from( fd_all );
            },
            std::ref(sigmaa_dif_map), std::ref(fd_all)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<float>& sigmaa_omit_fb, clipper::HKL_data<clipper::data32::F_phi>& fb_omit)
            {
                sigmaa_omit_fb.fft_from( fb_omit );
            },
            std::ref(sigmaa_omit_fb), std::ref(fb_omit)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<float>& sigmaa_omit_fd, clipper::HKL_data<clipper::data32::F_phi>& fd_omit)
            {
                sigmaa_omit_fd.fft_from( fd_omit );
            },
            std::ref(sigmaa_omit_fd), std::ref(fd_omit)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<float>& ligandmap, clipper::HKL_data<clipper::data32::F_phi>& fc_ligands_bsc)
            {
                ligandmap.fft_from( fc_ligands_bsc );
            },
            std::ref(ligandmap), std::ref(fc_ligands_bsc)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::HKL_data<clipper::data32::F_sigF>& fobs_scaled, clipper::ResolutionFn& wrk_scale_all, clipper::ResolutionFn& wrk_scale_omit,
                clipper::HKL_data<clipper::data32::F_phi>& fc_all_bsc, clipper::HKL_data<clipper::data32::F_phi>& fc_omit_bsc, HRI& ih, 
                double& Fo, double& Fc_all, double& Fc_omit, double& FobsFcalcAllSum, double& FobsFcalcSum, double& FobsSum)
            {
                for ( ih = fobs_scaled.first(); !ih.last(); ih.next() )
                {
                    if ( !fobs_scaled[ih].missing() )
                    {
                        Fo = fobs_scaled[ih].f();
                        Fc_all = sqrt ( wrk_scale_all.f(ih) ) * fc_all_bsc[ih].f() ;
                        Fc_omit = sqrt ( wrk_scale_omit.f(ih) ) * fc_omit_bsc[ih].f() ;
                        FobsFcalcSum += fabs( Fo - Fc_omit); // R factor calculation
                        FobsFcalcAllSum += fabs( Fo- Fc_all);
                        FobsSum += Fo;
                    }
                }
            },
            std::ref(fobs_scaled), std::ref(wrk_scale_all), std::ref(wrk_scale_omit), std::ref(fc_all_bsc), std::ref(fc_omit_bsc), std::ref(ih), std::ref(Fo), std::ref(Fc_all), std::ref(Fc_omit), std::ref(FobsFcalcAllSum), std::ref(FobsFcalcSum), std::ref(FobsSum)
            ));
        
        for (auto& r: thread_results)
            r.get();
        thread_results.clear();
    }
    else
    {
        sigmaa_all_map.fft_from( fb_all );  // calculate the maps
        sigmaa_dif_map.fft_from( fd_all );
        sigmaa_omit_fb.fft_from( fb_omit );
        sigmaa_omit_fd.fft_from( fd_omit );
        ligandmap.fft_from( fc_ligands_bsc );       // this is the map that will serve as Fc map for the RSCC calculation

        for ( ih = fobs_scaled.first(); !ih.last(); ih.next() )
        {
            if ( !fobs_scaled[ih].missing() )
            {
                Fo = fobs_scaled[ih].f();
                Fc_all = sqrt ( wrk_scale_all.f(ih) ) * fc_all_bsc[ih].f() ;
                Fc_omit = sqrt ( wrk_scale_omit.f(ih) ) * fc_omit_bsc[ih].f() ;
                FobsFcalcSum += fabs( Fo - Fc_omit); // R factor calculation
                FobsFcalcAllSum += fabs( Fo- Fc_all);
                FobsSum += Fo;
            }
        }
    }

    if (((FobsFcalcAllSum / FobsSum)*10) > hklinfo.resolution().limit() + 0.6)
        std::cout << " Warning: R-work is unusually high. Please ensure that your PDB file contains full B-factors instead of residuals after TLS refinement!" << std::endl;

    float difference = (FobsFcalcSum / FobsSum) - (FobsFcalcAllSum / FobsSum);

    if (( difference > 0.15 ) || (clipper::Util::is_nan((FobsFcalcSum / FobsSum))))
    {
        useSigmaa = true;

        std::cout << std::endl << " The studied portions of the model account for a very significant part of the data. Calculating RSCC against a regular 2mFo-DFc map" << std::endl;
    }

    if(useParallelism && nThreads >= 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        size_t sugars_per_thread = ligandList.size() / nThreads + 1;
        size_t start = 0, end;
        std::vector<std::future<void>> thread_results;
        for (size_t i = 0; i < nThreads; i++)
        {
            end = std::min(start + sugars_per_thread, ligandList.size());
            thread_results.push_back(std::async(std::launch::async, [](std::vector < clipper::MMonomer >& sugarList, clipper::String& path_to_model_file_clipper, std::vector<std::pair<clipper::String, clipper::MSugar>>& ligandList, 
            clipper::HKL_info& hklinfo,clipper::Grid_sampling& mygrid, clipper::Xmap<float>& sigmaa_all_map, clipper::Xmap<float>& sigmaa_omit_fb, clipper::Xmap<float>& sigmaa_omit_fd, clipper::Xmap<float>& ligandmap, clipper::MGlycology& mgl, 
            bool showGeom, float ipradius, int pos_slash, bool useSigmaa, bool rscc_best, size_t start, size_t end, bool debug_output)
            {
                for(size_t index = start; index < end; index++)
                {

                    float x,y,z,maxX,maxY,maxZ,minX,minY,minZ;
                    x=y=z=0.0;
                    maxX=maxY=maxZ=-999999.0;
                    minX=minY=minZ=999999.0;

                    for (int natom = 0; natom < sugarList[index].size(); natom++)
                    {
                        if(sugarList[index][natom].coord_orth().x() > maxX) maxX=sugarList[index][natom].coord_orth().x(); // calculation of the sugar centre
                        if(sugarList[index][natom].coord_orth().y() > maxY) maxY=sugarList[index][natom].coord_orth().y();
                        if(sugarList[index][natom].coord_orth().z() > maxZ) maxZ=sugarList[index][natom].coord_orth().z();
                        if(sugarList[index][natom].coord_orth().x() < minX) minX=sugarList[index][natom].coord_orth().x();
                        if(sugarList[index][natom].coord_orth().y() < minY) minY=sugarList[index][natom].coord_orth().y();
                        if(sugarList[index][natom].coord_orth().z() < minZ) minZ=sugarList[index][natom].coord_orth().z();
                    }

                    x = minX + ((maxX - minX)/2);
                    y = minY + ((maxY - minY)/2);
                    z = minZ + ((maxZ - minZ)/2);

                    // now calculate the correlation between the weighted experimental & calculated maps
                    // maps are scanned only inside a sphere containing the sugar for performance reasons,
                    // although RSCC and <RMS> are restricted to a mask surrounding the model

                    //////// mask calculation //////////
                    clipper::Xmap<float> mask( hklinfo.spacegroup(), hklinfo.cell(), mygrid );
                    clipper::EDcalc_mask<float> masker( ipradius );
                    masker(mask, sugarList[index].atom_list());

                    ////////////////////////////////////

                    clipper::Coord_orth origin(minX-2,minY-2,minZ-2);
                    clipper::Coord_orth destination(maxX+2,maxY+2,maxZ+2);


                    double accum = 0.0;
                    double corr_coeff = 0.0;
                    std::pair<double, double> rscc_and_accum;
                    if ( rscc_best ) {
                    rscc_and_accum = privateer::xray::calculate_rscc(sigmaa_all_map, sigmaa_omit_fb, ligandmap, mask, hklinfo, mygrid, origin, destination, useSigmaa);
                    }
                    else {
                    rscc_and_accum = privateer::xray::calculate_rscc(sigmaa_all_map, sigmaa_omit_fd, ligandmap, mask, hklinfo, mygrid, origin, destination, useSigmaa);
                    }
                    corr_coeff = rscc_and_accum.first;
                    accum = rscc_and_accum.second;

                    ligandList[index].second.set_rscc ( corr_coeff );
                    ligandList[index].second.set_accum_score ( accum );
                    // calculation of the mean densities of the calc (ligandmap) and weighted obs (sigmaamap) maps

                    std::vector < clipper::MGlycan > list_of_glycans = mgl.get_list_of_glycans();
                    bool found_in_tree = false;

                    for ( int i = 0 ; i < list_of_glycans.size() ; i++ )
                    {
                        std::vector < clipper::MSugar > list_of_sugars = list_of_glycans[i].get_sugars();

                        for ( int j = 0 ; j < list_of_sugars.size() ; j++ )
                        {
                            if ( list_of_sugars[j].id().trim() == ligandList[index].second.id().trim() )
                            {
                                if ( list_of_glycans[i].get_type() == "n-glycan" )
                                {
                                    ligandList[index].second.set_context ( "n-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "c-glycan" )
                                {
                                    if ( ligandList[index].second.type().trim() == "MAN" ) {
                                    if ( ligandList[index].second.conformation_name() == "1c4" ) {
                                        ligandList[index].second.override_conformation_diag ( true );
                                    }
                                    }
                                    ligandList[index].second.set_context ( "c-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "o-glycan" )
                                {
                                    ligandList[index].second.set_context ( "o-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "s-glycan" )
                                {
                                    ligandList[index].second.set_context ( "s-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "ligand" )
                                {
                                    ligandList[index].second.set_context ( "ligand" );
                                }
                                found_in_tree = true;
                                break;
                            }
                        }
                        if ( found_in_tree ) break;
                    }

                    if ( !found_in_tree )
                    {
                        ligandList[index].second.set_context ( "ligand" );
                    }


                    bool occupancy_check = false;
                    std::vector<clipper::MAtom> ringcomponents = ligandList[index].second.ring_members();

                    for ( int i = 0 ; i < ringcomponents.size() ; i++ )
                        if (privateer::util::get_altconformation(ringcomponents[i]) != ' ')
                            occupancy_check = true;

                    ligandList[index].second.set_occupancy_check ( occupancy_check );
                }
            },
            std::ref(sugarList), std::ref(path_to_model_file_clipper), std::ref(ligandList), std::ref(hklinfo), std::ref(mygrid), std::ref(sigmaa_all_map), std::ref(sigmaa_omit_fb), std::ref(sigmaa_omit_fd), std::ref(ligandmap), std::ref(mgl), showGeom, ipradius, pos_slash, useSigmaa, rscc_best, start, end, debug_output 
            ));
            start += sugars_per_thread;
        }

        for (auto& r: thread_results)
            r.get();
        
        thread_results.clear();
    }
    else
    {
        int processedMonomers = 0;
        for (int index = 0; index < ligandList.size(); index++)
        {
            float x,y,z,maxX,maxY,maxZ,minX,minY,minZ;
            x=y=z=0.0;
            maxX=maxY=maxZ=-999999.0;
            minX=minY=minZ=999999.0;

            for (int natom = 0; natom < sugarList[index].size(); natom++)
            {
                if(sugarList[index][natom].coord_orth().x() > maxX) maxX=sugarList[index][natom].coord_orth().x(); // calculation of the sugar centre
                if(sugarList[index][natom].coord_orth().y() > maxY) maxY=sugarList[index][natom].coord_orth().y();
                if(sugarList[index][natom].coord_orth().z() > maxZ) maxZ=sugarList[index][natom].coord_orth().z();
                if(sugarList[index][natom].coord_orth().x() < minX) minX=sugarList[index][natom].coord_orth().x();
                if(sugarList[index][natom].coord_orth().y() < minY) minY=sugarList[index][natom].coord_orth().y();
                if(sugarList[index][natom].coord_orth().z() < minZ) minZ=sugarList[index][natom].coord_orth().z();
            }

            x = minX + ((maxX - minX)/2);
            y = minY + ((maxY - minY)/2);
            z = minZ + ((maxZ - minZ)/2);

            // now calculate the correlation between the weighted experimental & calculated maps
            // maps are scanned only inside a sphere containing the sugar for performance reasons,
            // although RSCC and <RMS> are restricted to a mask surrounding the model

            //////// mask calculation //////////

            clipper::Xmap<float> mask( hklinfo.spacegroup(), hklinfo.cell(), mygrid );

            clipper::EDcalc_mask<float> masker( ipradius );
            masker(mask, sugarList[index].atom_list());

            ////////////////////////////////////

            clipper::Coord_orth origin(minX-2,minY-2,minZ-2);
            clipper::Coord_orth destination(maxX+2,maxY+2,maxZ+2);

            double accum = 0.0;
            double corr_coeff = 0.0;
            std::pair<double, double> rscc_and_accum;

            if ( rscc_best ) {
                rscc_and_accum = privateer::xray::calculate_rscc(sigmaa_all_map, sigmaa_omit_fb, ligandmap, mask, hklinfo, mygrid, origin, destination, useSigmaa);
            }
            else {
                rscc_and_accum = privateer::xray::calculate_rscc(sigmaa_all_map, sigmaa_omit_fd, ligandmap, mask, hklinfo, mygrid, origin, destination, useSigmaa);
            }

            corr_coeff = rscc_and_accum.first;
            accum = rscc_and_accum.second;

            ligandList[index].second.set_rscc ( corr_coeff );
            ligandList[index].second.set_accum_score ( accum );
            // calculation of the mean densities of the calc (ligandmap) and weighted obs (sigmaamap) maps

            std::vector < clipper::MGlycan > list_of_glycans = mgl.get_list_of_glycans();
            bool found_in_tree = false;

            for ( int i = 0 ; i < list_of_glycans.size() ; i++ )
            {
                std::vector < clipper::MSugar > list_of_sugars = list_of_glycans[i].get_sugars();

                for ( int j = 0 ; j < list_of_sugars.size() ; j++ )
                {
                    if ( list_of_sugars[j].id().trim() == ligandList[index].second.id().trim() )
                    {
                        if ( list_of_glycans[i].get_type() == "n-glycan" )
                        {
                            ligandList[index].second.set_context ( "n-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "c-glycan" )
                        {
                            if ( ligandList[index].second.type().trim() == "MAN" ) {
                                if ( ligandList[index].second.conformation_name() == "1c4" ) {
                                ligandList[index].second.override_conformation_diag ( true );
                                }
                            }
                            ligandList[index].second.set_context ( "c-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "o-glycan" )
                        {
                            ligandList[index].second.set_context ( "o-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "s-glycan" )
                        {
                            ligandList[index].second.set_context ( "s-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "ligand" )
                        {
                            ligandList[index].second.set_context ( "ligand" );
                        }
                        found_in_tree = true;
                        break;
                    }
                }
                if ( found_in_tree )
                    break;
            }

            if ( !found_in_tree )
            {
                ligandList[index].second.set_context ( "ligand" );
            }

            bool occupancy_check = false;
            std::vector<clipper::MAtom> ringcomponents = ligandList[index].second.ring_members();

            for ( int i = 0 ; i < ringcomponents.size() ; i++ )
                if (privateer::util::get_altconformation(ringcomponents[i]) != ' ')
                    occupancy_check = true;

            ligandList[index].second.set_occupancy_check ( occupancy_check );

            processedMonomers++;

            if(debug_output)
            {
                std::cout << std::endl;
                DBG << "Processed " << processedMonomers << "/" << ligandList.size() << " monomers..." << std::endl;
            }
        }
    }

        
    this->final_LigandsOnly = ligandsOnly;
    this->finalLigandList = ligandList;
    this->sugar_summary_of_experimental_data = generate_sugar_experimental_data_summary(finalLigandList);
    this->ligand_summary_of_experimental_data = generate_sugar_experimental_data_summary(final_LigandsOnly);
}

// private methods //
pybind11::list privateer::pyanalysis::XRayData::generate_sugar_experimental_data_summary(std::vector<std::pair< clipper::String , clipper::MSugar>>& finalLigandList)
{
    pybind11::list output; 

    for(int index = 0; index < finalLigandList.size(); index++)
    {
        std::string ccdCode = finalLigandList[index].second.type().c_str();
        std::string chainID = finalLigandList[index].first;
        int sugarID = index;
        std::string pdbID = finalLigandList[index].second.id().trim();
        float RSCC = finalLigandList[index].second.get_rscc();
        float accum = finalLigandList[index].second.get_accum();
        bool occupancy_check = finalLigandList[index].second.get_occupancy_check();

        std::string sugardiagnostic;
        if(finalLigandList[index].second.is_sane())
        {
            if(!finalLigandList[index].second.ok_with_conformation())
            {
                sugardiagnostic = "check";
            }
            else
                sugardiagnostic = "yes";
        }
        else
            sugardiagnostic = "no";

        auto currentSugar = pybind11::dict ("three_letter_code"_a=ccdCode, "Chain"_a=chainID, "sugar_index_internal"_a=sugarID, "PDB_ID"_a=pdbID, "RSCC"_a=RSCC, "mFo"_a=accum, "privateer_diagnostic"_a=sugardiagnostic, "occupancy_check_required"_a=occupancy_check);
        output.append(currentSugar);
    }
    return output;
}
// private methods end //

///////////////////////////////////////////////// Class XrayData END ////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Class CryoEMData ////////////////////////////////////////////////////////////////////

void privateer::pyanalysis::CryoEMData::read_from_file( std::string& path_to_mrc_file, std::string& path_to_model_file, float resolution, float ipradius, int nThreads, bool debug_output) 
{
    this->debug_output = debug_output;

    bool useSigmaa = false;
    int detectedThreads = std::thread::hardware_concurrency();
    bool useParallelism = true;
    bool showGeom = false;
    
    if ( resolution == -1)
    {
        throw std::invalid_argument( "\nFATAL: An MRC file was inputted, but no resolution value was given. To import a Cryo-EM map please use input resolution value as the third argument!" );
    }
    
    if(nThreads < 0)
        nThreads = detectedThreads;
    else if(nThreads < 2 && nThreads > -1)
    {
        useParallelism = false;
    }
    else if(nThreads > detectedThreads)
    {
        std::cout << "Error: More cores/threads were inputted as an argument, than detected on the system." 
        << "\n\tNumber of Available Cores/Threads detected on the system: " << detectedThreads 
        << "\n\tNumber of Cores/Threads requested via input argument: " << nThreads << "." << std::endl;

        throw std::invalid_argument( "Number of inputted threads exceed the number of detected threads." );
    }

    if(useParallelism)
    {
        std::cout << std::endl << "THREADING: Privateer will use " << nThreads << " of the " << std::thread::hardware_concurrency() << " available threads..." << std::endl;
    }

    if(path_to_model_file == "undefined" || path_to_model_file == "")
    {
        throw std::invalid_argument( "No path was provided for model file input! Aborting." );
    }

    if(path_to_mrc_file == "undefined" || path_to_mrc_file == "")
    {
        throw std::invalid_argument( "No path was provided for .mtz file input! Aborting." );
    }

    int pos_slash = path_to_model_file.rfind("/");

    clipper::String path_to_model_file_clipper = path_to_model_file;
    this->path_to_model_file=path_to_model_file;
    clipper::String path_to_mrc_file_clipper = path_to_mrc_file;
    this->resolution = resolution;
    clipper::Xmap<double> cryo_em_map;
    clipper::MMDBfile mfile;
    clipper::CCP4MAPfile mrcin;
    clipper::HKL_info hklinfo;

    privateer::cryo_em::read_cryoem_map ( path_to_mrc_file_clipper, hklinfo, cryo_em_map, mrcin, resolution);
    privateer::util::read_coordinate_file_mrc (mfile, mmol, path_to_model_file_clipper, cryo_em_map, true);

    clipper::HKL_data<clipper::data32::F_sigF> fobs;            // allocate space for F and sigF
    clipper::HKL_data<clipper::data32::F_phi> fc_cryoem_obs;    // allocate space for cryoEM calculated structure factors, that acts as observed data.
    clipper::HKL_data<clipper::data32::F_phi> fc_all_cryoem_data; // allocate space for entire cryoEM calculated model, that acts as calculated data.
    clipper::HKL_data<clipper::data32::F_phi> fc_ligands_only_cryoem_data; // allocate space for calculated cryoEM model of ligands only, that acts as calculated data.

    fobs = clipper::HKL_data<clipper::data32::F_sigF> ( hklinfo );
    fc_cryoem_obs = clipper::HKL_data<clipper::data32::F_phi> ( hklinfo, cryo_em_map.cell() );
    fc_all_cryoem_data = clipper::HKL_data<clipper::data32::F_phi> ( hklinfo );
    fc_ligands_only_cryoem_data = clipper::HKL_data<clipper::data32::F_phi> ( hklinfo );
    
    cryo_em_map.fft_to(fc_cryoem_obs);

    this->hklinfo = hklinfo;

    std::vector<std::pair<clipper::String, clipper::MSugar>> ligandsOnly;
    clipper::Atom_list mainAtoms;
    clipper::Atom_list ligandAtoms;
    clipper::Atom_list allAtoms;

    std::vector<std::pair< clipper::String , clipper::MSugar> > ligandList; // we store the Chain ID and create an MSugar to be scored
    std::vector<clipper::MMonomer> sugarList; // store the original MMonomer

    const clipper::MAtomNonBond& manb = clipper::MAtomNonBond( mmol, 1.0 ); // was 1.0

    clipper::MGlycology mgl = clipper::MGlycology(mmol, manb, debug_output, "undefined");

    for ( int p = 0; p < mmol.size(); p++ )
    {
        for ( int m = 0; m < mmol[p].size(); m++ )
        {
            if ( clipper::MDisaccharide::search_disaccharides(mmol[p][m].type().c_str()) != -1 ) // treat disaccharide
            {
                clipper::MDisaccharide md(mmol, manb, mmol[p][m], debug_output );
                sugarList.push_back ( mmol[p][m] );
                sugarList.push_back ( mmol[p][m] );
                clipper::String id = mmol[p].id();
                id.resize(1);

                ligandList.push_back ( std::pair < clipper::String, clipper::MSugar> (id, md.get_first_sugar()));
                ligandList.push_back ( std::pair < clipper::String, clipper::MSugar> (id, md.get_second_sugar()));

                if (( md.get_first_sugar().type_of_sugar() == "unsupported" ) || ( md.get_second_sugar().type_of_sugar() == "unsupported" ) )
                {
                    throw std::invalid_argument( "Error: strangely, at least one of the sugars in the supplied PDB file is missing required atoms. Stopping..." );
                }

                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    ligandAtoms.push_back(mmol[p][m][id]);  // add the ligand atoms to a second array
                    allAtoms.push_back(mmol[p][m][id]);
                }

            }
            else if ( !clipper::MSugar::search_database(mmol[p][m].type().c_str()) )
            {
                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    mainAtoms.push_back(mmol[p][m][id]); // cycle through atoms and copy them
                    allAtoms.push_back(mmol[p][m][id]);
                }
            }
            else // it's one of the sugars contained in the database
            {
                clipper::MSugar msug, msug_b;

                std::vector <char> conformers = privateer::util::number_of_conformers(mmol[p][m]);

                // if(debug_output)
                //     std::cout << "number of alternate conformations: " << conformers.size() << std::endl;
                // }

                int n_conf = conformers.size();

                if ( n_conf > 0 )
                {
                    if ( n_conf == 1 )
                        msug   = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[0]);
                    else
                    {
                        msug   = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[0]);
                        msug_b = clipper::MSugar(mmol, mmol[p][m], manb, debug_output, conformers[1]);
                    }

                }
                else
                {
                    msug = clipper::MSugar(mmol, mmol[p][m], manb, debug_output);
                }

                sugarList.push_back(mmol[p][m]);
                clipper::String id = mmol[p].id();
                id.resize(1);

                ligandList.push_back(std::pair<clipper::String, clipper::MSugar> (id, msug));
                // add both conformers if the current monomer contains more than one
                if ( n_conf == 2 )
                {
                    ligandList.push_back(std::pair<clipper::String, clipper::MSugar> (id, msug_b));
                    sugarList.push_back(mmol[p][m]);
                }

                if ( msug.type_of_sugar() == "unsupported" )
                {
                    throw std::invalid_argument( "Error: strangely, at least one of the sugars in the supplied PDB file is missing required atoms. Stopping..." );
                }

                for (int id = 0; id < mmol[p][m].size(); id++ )
                {
                    ligandAtoms.push_back(mmol[p][m][id]);  // add the ligand atoms to a second array
                    allAtoms.push_back(mmol[p][m][id]);
                }
            }
        }
    }

    privateer::cryo_em::calculate_sfcs_of_fc_maps ( fc_all_cryoem_data, fc_ligands_only_cryoem_data, allAtoms, ligandAtoms, nThreads, useParallelism, debug_output);

    std::cout << "done." << std::endl << "Computing Fo-DFc map... ";
    fflush(0);


    clipper::Grid_sampling mygrid( cryo_em_map.grid_asu().nu(), cryo_em_map.grid_asu().nv(), cryo_em_map.grid_asu().nw() );
    clipper::Xmap<double> cryo_em_dif_map_all( hklinfo.spacegroup(), hklinfo.cell(), mygrid );          // define sigmaa diff  map
    clipper::Xmap<double> cryo_em_twotimes_obs_dif_map_all( hklinfo.spacegroup(), hklinfo.cell(), mygrid );          // define sigmaa diff  map
    clipper::Xmap<double> ligandmap( hklinfo.spacegroup(), hklinfo.cell(), mygrid );

    // scale data and flag R-free

    HRI ih;

    clipper::HKL_data<clipper::data32::F_phi> difference_coefficients( hklinfo );

    bool difference_map_sfc_generated = privateer::cryo_em::generate_output_map_coefficients(difference_coefficients, fc_cryoem_obs, fc_all_cryoem_data, hklinfo);

    if (!difference_map_sfc_generated)
    {
        std::cout << "\n\nUnable to calculate Fo-DFc structure factors for cryoEM map input. Exiting..." << std::endl << std::endl;
    }

    clipper::Xmap<double> modelmap( hklinfo.spacegroup(), hklinfo.cell(), mygrid );

    if(useParallelism && nThreads >= 2)
    {
        std::vector<std::future<void>> thread_results;
        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<double>& cryo_em_dif_map_all, clipper::HKL_data<clipper::data32::F_phi>& difference_coefficients)
            {
                cryo_em_dif_map_all.fft_from( difference_coefficients );
            },
            std::ref(cryo_em_dif_map_all), std::ref(difference_coefficients)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<double>& modelmap, clipper::HKL_data<clipper::data32::F_phi>& fc_all_cryoem_data)
            {
                modelmap.fft_from( fc_all_cryoem_data );
            },
            std::ref(modelmap), std::ref(fc_all_cryoem_data)
            ));

        thread_results.push_back(std::async(std::launch::async, 
            [](clipper::Xmap<double>& ligandmap, clipper::HKL_data<clipper::data32::F_phi>& fc_ligands_only_cryoem_data)
            {
                ligandmap.fft_from( fc_ligands_only_cryoem_data );
            },
            std::ref(ligandmap), std::ref(fc_ligands_only_cryoem_data)
            ));
        
        for (auto& r: thread_results)
            r.get();

        thread_results.clear();
    }
    else
    {
        cryo_em_dif_map_all.fft_from( difference_coefficients );

        modelmap.fft_from( fc_all_cryoem_data );

        ligandmap.fft_from( fc_ligands_only_cryoem_data );       // this is the map that will serve as Fc map for the RSCC calculation
    }
    
    if(useParallelism && nThreads >= 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        size_t sugars_per_thread = ligandList.size() / nThreads + 1;
        size_t start = 0, end;
        std::vector<std::future<void>> thread_results;
        for (size_t i = 0; i < nThreads; i++)
        {
            end = std::min(start + sugars_per_thread, ligandList.size());
            thread_results.push_back(std::async(std::launch::async, [](std::vector < clipper::MMonomer >& sugarList, clipper::String& input_model, std::vector<std::pair<clipper::String, clipper::MSugar>>& ligandList, 
            clipper::HKL_info& hklinfo,clipper::Grid_sampling& mygrid, clipper::Xmap<double>& cryo_em_map, clipper::Xmap<double>& ligandmap, clipper::MGlycology& mgl, 
            bool showGeom, float ipradius, int pos_slash, size_t start, size_t end, bool debug_output)
            {
                for(size_t index = start; index < end; index++)
                {
                    float x,y,z,maxX,maxY,maxZ,minX,minY,minZ;
                    x=y=z=0.0;
                    maxX=maxY=maxZ=-999999.0;
                    minX=minY=minZ=999999.0;

                    for (int natom = 0; natom < sugarList[index].size(); natom++)
                    {
                        if(sugarList[index][natom].coord_orth().x() > maxX) maxX=sugarList[index][natom].coord_orth().x(); // calculation of the sugar centre
                        if(sugarList[index][natom].coord_orth().y() > maxY) maxY=sugarList[index][natom].coord_orth().y();
                        if(sugarList[index][natom].coord_orth().z() > maxZ) maxZ=sugarList[index][natom].coord_orth().z();
                        if(sugarList[index][natom].coord_orth().x() < minX) minX=sugarList[index][natom].coord_orth().x();
                        if(sugarList[index][natom].coord_orth().y() < minY) minY=sugarList[index][natom].coord_orth().y();
                        if(sugarList[index][natom].coord_orth().z() < minZ) minZ=sugarList[index][natom].coord_orth().z();
                    }

                    x = minX + ((maxX - minX)/2);
                    y = minY + ((maxY - minY)/2);
                    z = minZ + ((maxZ - minZ)/2);


                    // now calculate the correlation between the weighted experimental & calculated maps
                    // maps are scanned only inside a sphere containing the sugar for performance reasons,
                    // although RSCC and <RMS> are restricted to a mask surrounding the model


                    //////// mask calculation //////////

                    clipper::Xmap<double> mask( hklinfo.spacegroup(), hklinfo.cell(), mygrid );

                    clipper::EDcalc_mask<double> masker( ipradius );
                    masker(mask, sugarList[index].atom_list());

                    ////////////////////////////////////

                    clipper::Coord_orth origin(minX-2,minY-2,minZ-2);
                    clipper::Coord_orth destination(maxX+2,maxY+2,maxZ+2);

                    double accum = 0.0;
                    double corr_coeff = 0.0;
                    std::pair<double, double> rscc_and_accum;


                    rscc_and_accum = privateer::cryo_em::calculate_rscc(cryo_em_map, ligandmap, mask, hklinfo, mygrid, origin, destination);

                    corr_coeff = rscc_and_accum.first;
                    accum = rscc_and_accum.second;

                    ligandList[index].second.set_rscc ( corr_coeff );
                    ligandList[index].second.set_accum_score ( accum );
                    ///////////// here we deal with the sugar /////////////

                    std::vector < clipper::MGlycan > list_of_glycans = mgl.get_list_of_glycans();
                    bool found_in_tree = false;

                    for ( int i = 0 ; i < list_of_glycans.size() ; i++ )
                    {
                        std::vector < clipper::MSugar > list_of_sugars = list_of_glycans[i].get_sugars();

                        for ( int j = 0 ; j < list_of_sugars.size() ; j++ )
                        {
                            if ( list_of_sugars[j].id().trim() == ligandList[index].second.id().trim() )
                            {
                                if ( list_of_glycans[i].get_type() == "n-glycan" )
                                {
                                    ligandList[index].second.set_context ( "n-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "c-glycan" )
                                {
                                    if ( ligandList[index].second.type().trim() == "MAN" ) {
                                    if ( ligandList[index].second.conformation_name() == "1c4" ) {
                                        ligandList[index].second.override_conformation_diag ( true );
                                    }
                                    }
                                    ligandList[index].second.set_context ( "c-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "o-glycan" )
                                {
                                    ligandList[index].second.set_context ( "o-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "s-glycan" )
                                {
                                    ligandList[index].second.set_context ( "s-glycan" );
                                }
                                else if ( list_of_glycans[i].get_type() == "ligand" )
                                {
                                    ligandList[index].second.set_context ( "ligand" );
                                }
                                found_in_tree = true;
                                break;
                            }
                        }
                        if ( found_in_tree ) break;
                    }

                    if ( !found_in_tree )
                    {
                        ligandList[index].second.set_context ( "ligand" );
                    }

                    bool occupancy_check = false;
                    std::vector<clipper::MAtom> ringcomponents = ligandList[index].second.ring_members();

                    for ( int i = 0 ; i < ringcomponents.size() ; i++ )
                        if (privateer::util::get_altconformation(ringcomponents[i]) != ' ')
                            occupancy_check = true;

                    ligandList[index].second.set_occupancy_check ( occupancy_check );
                }
            },
            std::ref(sugarList), std::ref(path_to_model_file_clipper), std::ref(ligandList), std::ref(hklinfo), std::ref(mygrid), std::ref(cryo_em_map), std::ref(ligandmap), std::ref(mgl), showGeom, ipradius, pos_slash, start, end, debug_output 
            ));
            start += sugars_per_thread;
        }
        for (auto& r: thread_results)
            r.get();
        thread_results.clear();
    }
    else
    {
        int processedMonomers = 0;
        for (int index = 0; index < ligandList.size(); index++)
        {
            float x,y,z,maxX,maxY,maxZ,minX,minY,minZ;
            x=y=z=0.0;
            maxX=maxY=maxZ=-999999.0;
            minX=minY=minZ=999999.0;

            for (int natom = 0; natom < sugarList[index].size(); natom++)
            {
                if(sugarList[index][natom].coord_orth().x() > maxX) maxX=sugarList[index][natom].coord_orth().x(); // calculation of the sugar centre
                if(sugarList[index][natom].coord_orth().y() > maxY) maxY=sugarList[index][natom].coord_orth().y();
                if(sugarList[index][natom].coord_orth().z() > maxZ) maxZ=sugarList[index][natom].coord_orth().z();
                if(sugarList[index][natom].coord_orth().x() < minX) minX=sugarList[index][natom].coord_orth().x();
                if(sugarList[index][natom].coord_orth().y() < minY) minY=sugarList[index][natom].coord_orth().y();
                if(sugarList[index][natom].coord_orth().z() < minZ) minZ=sugarList[index][natom].coord_orth().z();
            }

            x = minX + ((maxX - minX)/2);
            y = minY + ((maxY - minY)/2);
            z = minZ + ((maxZ - minZ)/2);


            // now calculate the correlation between the weighted experimental & calculated maps
            // maps are scanned only inside a sphere containing the sugar for performance reasons,
            // although RSCC and <RMS> are restricted to a mask surrounding the model


            //////// mask calculation //////////

            clipper::Xmap<double> mask( hklinfo.spacegroup(), hklinfo.cell(), mygrid );

            clipper::EDcalc_mask<double> masker( ipradius );
            masker(mask, sugarList[index].atom_list());

            ////////////////////////////////////

            clipper::Coord_orth origin(minX-2,minY-2,minZ-2);
            clipper::Coord_orth destination(maxX+2,maxY+2,maxZ+2);

            double accum = 0.0;
            double corr_coeff = 0.0;
            std::pair<double, double> rscc_and_accum;


            rscc_and_accum = privateer::cryo_em::calculate_rscc(cryo_em_map, ligandmap, mask, hklinfo, mygrid, origin, destination);

            corr_coeff = rscc_and_accum.first;
            accum = rscc_and_accum.second;

            ligandList[index].second.set_rscc ( corr_coeff );
            ligandList[index].second.set_accum_score ( accum );
            ///////////// here we deal with the sugar /////////////

            std::vector < clipper::MGlycan > list_of_glycans = mgl.get_list_of_glycans();
            bool found_in_tree = false;

            for ( int i = 0 ; i < list_of_glycans.size() ; i++ )
            {
                std::vector < clipper::MSugar > list_of_sugars = list_of_glycans[i].get_sugars();

                for ( int j = 0 ; j < list_of_sugars.size() ; j++ )
                {
                    if ( list_of_sugars[j].id().trim() == ligandList[index].second.id().trim() )
                    {
                        if ( list_of_glycans[i].get_type() == "n-glycan" )
                        {
                            ligandList[index].second.set_context ( "n-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "c-glycan" )
                        {
                            if ( ligandList[index].second.type().trim() == "MAN" ) {
                                if ( ligandList[index].second.conformation_name() == "1c4" ) {
                                ligandList[index].second.override_conformation_diag ( true );
                                }
                            }
                            ligandList[index].second.set_context ( "c-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "o-glycan" )
                        {
                            ligandList[index].second.set_context ( "o-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "s-glycan" )
                        {
                            ligandList[index].second.set_context ( "s-glycan" );
                        }
                        else if ( list_of_glycans[i].get_type() == "ligand" )
                        {
                            ligandList[index].second.set_context ( "ligand" );
                        }
                        found_in_tree = true;
                        break;
                    }
                }
                if ( found_in_tree ) break;
            }

            if ( !found_in_tree )
            {
                ligandList[index].second.set_context ( "ligand" );
            }

            bool occupancy_check = false;
            std::vector<clipper::MAtom> ringcomponents = ligandList[index].second.ring_members();

            for ( int i = 0 ; i < ringcomponents.size() ; i++ )
                if (privateer::util::get_altconformation(ringcomponents[i]) != ' ')
                    occupancy_check = true;

            ligandList[index].second.set_occupancy_check ( occupancy_check );

            processedMonomers++;

            if(debug_output)
            {
                std::cout << std::endl;
                DBG << "Processed " << processedMonomers << "/" << ligandList.size() << " monomers..." << std::endl;
            }

            if(debug_output) DBG << "Processed " << processedMonomers << "/" << ligandList.size() << " monomers..." << std::endl;
        }
    }
        
    this->final_LigandsOnly = ligandsOnly;
    this->finalLigandList = ligandList;
    this->sugar_summary_of_experimental_data = generate_sugar_experimental_data_summary(finalLigandList);
    this->ligand_summary_of_experimental_data = generate_sugar_experimental_data_summary(final_LigandsOnly);
}

// private methods //
pybind11::list privateer::pyanalysis::CryoEMData::generate_sugar_experimental_data_summary(std::vector<std::pair< clipper::String , clipper::MSugar>>& finalLigandList)
{
    pybind11::list output; 

    for(int index = 0; index < finalLigandList.size(); index++)
    {
        std::string ccdCode = finalLigandList[index].second.type().c_str();
        std::string chainID = finalLigandList[index].first;
        int sugarID = index;
        std::string pdbID = finalLigandList[index].second.id().trim();
        float RSCC = finalLigandList[index].second.get_rscc();
        float accum = finalLigandList[index].second.get_accum();
        bool occupancy_check = finalLigandList[index].second.get_occupancy_check();

        std::string sugardiagnostic;
        if(finalLigandList[index].second.is_sane())
        {
            if(!finalLigandList[index].second.ok_with_conformation())
            {
                sugardiagnostic = "check";
            }
            else
                sugardiagnostic = "yes";
        }
        else
            sugardiagnostic = "no";

        auto currentSugar = pybind11::dict ("three_letter_code"_a=ccdCode, "Chain"_a=chainID, "sugar_index_internal"_a=sugarID, "PDB_ID"_a=pdbID, "RSCC"_a=RSCC, "mFo"_a=accum, "privateer_diagnostic"_a=sugardiagnostic, "occupancy_check_required"_a=occupancy_check);
        output.append(currentSugar);
    }
    return output;
}
// private methods end //

///////////////////////////////////////////////// Class CryoEMData END ////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Class OfflineGlycomicsDatabase ////////////////////////////////////////////////////////////////////
void privateer::pyanalysis::OfflineGlycomicsDatabase::import_json_file( std::string& path_to_input_file )
{
    std::vector<privateer::json::GlycomicsDatabase> glycomics_database = privateer::json::read_json_file_for_glycomics_database(path_to_input_file);
    this->glytoucanglyconnectdatabase = glycomics_database;
}
///////////////////////////////////////////////// Class OfflineGlycomicsDatabase END ////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Class OfflineTorsionsDatabase ////////////////////////////////////////////////////////////////////
void privateer::pyanalysis::OfflineTorsionsDatabase::import_json_file( std::string& path_to_input_file )
{
    std::vector<privateer::json::TorsionsDatabase> torsions_database = privateer::json::read_json_file_for_torsions_database(path_to_input_file);
    this->torsionsdatabase = torsions_database;
}
///////////////////////////////////////////////// Class OfflineTorsionsDatabase END ////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////// PYBIND11 BINDING DEFINITIONS ////////////////////////////////////////////////////////////////////
namespace py = pybind11;
namespace pa = privateer::pyanalysis;

void init_pyanalysis(py::module& m)
{
    // Need to pa::GlycosylationComposition_memorysafe(and probably pa::GlycanStructure_memorysafe) class or something like that for them huge ribosomes that
    // cause the Python process to hog all the Computer's memory. 
    // Basically change the paradigm from compute everything first and then rely on getters to call getter that then initiates a computation
    py::class_<pa::GlycosylationComposition>(m, "GlycosylationComposition")
        .def(py::init<>())
        .def(py::init<std::string&, std::string, bool>(), py::arg("path_to_model_file")="undefined", py::arg("expression_system")="undefined", py::arg("debug_output")=false)
        .def(py::init<std::string&, std::string&, std::string&, int, float, std::string, bool>(), py::arg("path_to_model_file")="undefined", py::arg("path_to_mtz_file")="undefined", py::arg("input_column_fobs_user")="NONE", py::arg("nThreads")=-1, py::arg("ipradius")=2.5, py::arg("expression_system")="undefined", py::arg("debug_output")=false)
        .def(py::init<std::string&, std::string&, float, int, float, std::string, bool>(), py::arg("path_to_model_file")="undefined", py::arg("path_to_mrc_file")="undefined", py::arg("resolution")=-1, py::arg("nThreads")=-1, py::arg("ipradius")=2.5, py::arg("expression_system")="undefined", py::arg("debug_output")=false)
        .def("get_path_of_model_file_used",  &pa::GlycosylationComposition::get_path_of_model_file_used)
        .def("get_expression_system_used",  &pa::GlycosylationComposition::get_expression_system_used)
        .def("get_number_of_glycan_chains_detected",  &pa::GlycosylationComposition::get_number_of_glycan_chains_detected)
        .def("get_summary_of_detected_glycans",  &pa::GlycosylationComposition::get_summary_of_detected_glycans)
        .def("get_glycan",  &pa::GlycosylationComposition::get_glycan)
        .def("get_ligands",  &pa::GlycosylationComposition::get_ligands)
        .def("get_torsions_summary", &pa::GlycosylationComposition::get_torsions_summary)
        .def("update_with_experimental_data", static_cast<void (pa::GlycosylationComposition::*)(pa::XRayData&)>(&pa::GlycosylationComposition::update_with_experimental_data), "Update model with X-Ray Crystallography Data")
        .def("update_with_experimental_data", static_cast<void (pa::GlycosylationComposition::*)(pa::CryoEMData&)>(&pa::GlycosylationComposition::update_with_experimental_data), "Update model with CryoEM Data")
        .def("check_if_updated_with_experimental_data",  &pa::GlycosylationComposition::check_if_updated_with_experimental_data);

    py::class_<pa::GlycosylationComposition_memsafe>(m, "GlycosylationComposition_memsafe")
        .def(py::init<>())
        .def(py::init<std::string&, std::string, bool>(), py::arg("path_to_model_file")="undefined", py::arg("expression_system")="undefined", py::arg("debug_output")=false)
        .def("get_path_of_model_file_used",  &pa::GlycosylationComposition_memsafe::get_path_of_model_file_used)
        .def("get_expression_system_used",  &pa::GlycosylationComposition_memsafe::get_expression_system_used)
        .def("get_number_of_glycan_chains_detected",  &pa::GlycosylationComposition_memsafe::get_number_of_glycan_chains_detected)
        .def("get_summary_of_detected_glycans",  &pa::GlycosylationComposition_memsafe::get_summary_of_detected_glycans)
        .def("get_glycan",  &pa::GlycosylationComposition_memsafe::get_glycan)
        .def("get_torsions_summary", &pa::GlycosylationComposition_memsafe::get_torsions_summary);

    py::class_<pa::GlycanStructure>(m, "GlycanStructure")
        .def(py::init<>())
        .def(py::init<const clipper::MGlycology&, const int>())
        .def(py::init<const clipper::MGlycology&, const int, privateer::pyanalysis::GlycosylationComposition&>())
        .def(py::init<const clipper::MGlycology&, const int, privateer::pyanalysis::GlycosylationComposition&, std::vector<std::pair< clipper::String , clipper::MSugar>>&>())
        .def("get_glycan_id", &pa::GlycanStructure::get_glycan_id)
        .def("get_total_number_of_sugars", &pa::GlycanStructure::get_total_number_of_sugars)
        .def("get_wurcs_notation", &pa::GlycanStructure::get_wurcs_notation)
        .def("get_unique_monosaccharide_codes", &pa::GlycanStructure::get_unique_monosaccharide_codes)
        .def("get_total_of_glycosidic_bonds", &pa::GlycanStructure::get_total_of_glycosidic_bonds)
        .def("get_glycosylation_type", &pa::GlycanStructure::get_glycosylation_type)
        .def("get_root_sugar_chain_id", &pa::GlycanStructure::get_root_sugar_chain_id)
        .def("get_root_info", &pa::GlycanStructure::get_root_info)
        .def("get_protein_glycan_linkage_torsions", &pa::GlycanStructure::get_protein_glycan_linkage_torsions)
        .def("get_glycan_summary", &pa::GlycanStructure::get_glycan_summary)
        .def("get_monosaccharide", &pa::GlycanStructure::get_monosaccharide)
        .def("get_all_monosaccharides", &pa::GlycanStructure::get_all_monosaccharides)
        .def("query_glycomics_database", &pa::GlycanStructure::query_glycomics_database, "Function to query GlyTouCan and GlyConnect databases with a possibility to return closest matches detected on GlyConnect",
        py::arg("importedDatabase"), py::arg("returnClosestMatches") = true, py::arg("returnAllPossiblePermutations") = false, py::arg("nThreads") = -1)
        .def("get_torsions_summary", &pa::GlycanStructure::get_torsions_summary)
        .def("get_SNFG_strings", &pa::GlycanStructure::get_SNFG_strings, "Returns Privateer generated SNFG representations in SVG string that later can be parsed through Python",
        py::arg("includeClosestMatches") = true)
        .def("update_with_experimental_data", static_cast<void (pa::GlycanStructure::*)(pa::XRayData&)>(&pa::GlycanStructure::update_with_experimental_data), "Update glycan with X-Ray Crystallography Data")
        .def("update_with_experimental_data", static_cast<void (pa::GlycanStructure::*)(pa::CryoEMData&)>(&pa::GlycanStructure::update_with_experimental_data), "Update glycan with CryoEM Data")
        .def("check_if_updated_with_experimental_data", &pa::GlycanStructure::check_if_updated_with_experimental_data);

    py::class_<pa::CarbohydrateStructure>(m, "CarbohydrateStructure")
        .def(py::init<>())
        .def(py::init<clipper::MGlycan&, const int, const int, privateer::pyanalysis::GlycosylationComposition&, privateer::pyanalysis::GlycanStructure&>())
        .def(py::init<clipper::MGlycan&, const int, const int, privateer::pyanalysis::GlycosylationComposition&, privateer::pyanalysis::GlycanStructure&, std::vector<clipper::MSugar>&>())
        .def(py::init<const int, std::vector<std::pair<clipper::String, clipper::MSugar>>&, privateer::pyanalysis::GlycosylationComposition&, bool>())
        .def(py::self == py::self)
        .def("get_sugar_summary", &pa::CarbohydrateStructure::get_sugar_summary)
        .def("get_sugar_id", &pa::CarbohydrateStructure::get_sugar_id)
        .def("get_glycan_id", &pa::CarbohydrateStructure::get_glycan_id)
        .def("get_seqnum", &pa::CarbohydrateStructure::get_seqnum)
        .def("get_sugar_pdb_id", &pa::CarbohydrateStructure::get_sugar_pdb_id)
        .def("get_sugar_pdb_chain", &pa::CarbohydrateStructure::get_sugar_pdb_chain)
        .def("get_conformation_name", &pa::CarbohydrateStructure::get_conformation_name)
        .def("get_conformation_name_iupac", &pa::CarbohydrateStructure::get_conformation_name_iupac)
        .def("get_puckering_amplitude", &pa::CarbohydrateStructure::get_puckering_amplitude)
        .def("get_anomer", &pa::CarbohydrateStructure::get_anomer)
        .def("get_handedness", &pa::CarbohydrateStructure::get_handedness)
        .def("get_denomination", &pa::CarbohydrateStructure::get_denomination)
        .def("get_ring_cardinality", &pa::CarbohydrateStructure::get_ring_cardinality)
        .def("get_cremer_pople_params", &pa::CarbohydrateStructure::get_cremer_pople_params)
        .def("is_sane", &pa::CarbohydrateStructure::is_sane)
        .def("get_privateer_diagnostic", &pa::CarbohydrateStructure::get_privateer_diagnostic)
        .def("get_name_full", &pa::CarbohydrateStructure::get_name_full)
        .def("get_name_short", &pa::CarbohydrateStructure::get_name_short)
        .def("get_type", &pa::CarbohydrateStructure::get_type)
        .def("get_ring_angles", &pa::CarbohydrateStructure::get_ring_angles)
        .def("get_ring_bonds", &pa::CarbohydrateStructure::get_ring_bonds)
        .def("get_ring_torsion", &pa::CarbohydrateStructure::get_ring_torsion)
        .def("get_ring_bond_rmsd", &pa::CarbohydrateStructure::get_ring_bond_rmsd)
        .def("get_ring_angle_rmsd", &pa::CarbohydrateStructure::get_ring_angle_rmsd)
        .def("get_bfactor", &pa::CarbohydrateStructure::get_bfactor)
        .def("is_supported", &pa::CarbohydrateStructure::is_supported)
        .def("ok_with_ring", &pa::CarbohydrateStructure::ok_with_ring)
        .def("ok_with_bonds_rmsd", &pa::CarbohydrateStructure::ok_with_bonds_rmsd)
        .def("ok_with_angles_rmsd", &pa::CarbohydrateStructure::ok_with_angles_rmsd)
        .def("ok_with_anomer", &pa::CarbohydrateStructure::ok_with_anomer)
        .def("ok_with_chirality", &pa::CarbohydrateStructure::ok_with_chirality)
        .def("ok_with_conformation", &pa::CarbohydrateStructure::ok_with_conformation)
        .def("ok_with_puckering", &pa::CarbohydrateStructure::ok_with_puckering)
        .def("get_sugar_rscc", &pa::CarbohydrateStructure::get_sugar_rscc)
        .def("get_sugar_accum", &pa::CarbohydrateStructure::get_sugar_accum)
        .def("get_sugar_occupancy_check", &pa::CarbohydrateStructure::get_sugar_occupancy_check)
        .def("get_glycosylation_type", &pa::CarbohydrateStructure::get_glycosylation_context)
        .def("get_ring_atom_names", &pa::CarbohydrateStructure::get_ring_atom_names)
        .def("get_wurcs_uniqres_code", &pa::CarbohydrateStructure::get_wurcs_residue_code)
        .def("get_number_of_connections", &pa::CarbohydrateStructure::get_number_of_connections)
        .def("get_sugar_linkage_info", &pa::CarbohydrateStructure::get_sugar_linkage_info)
        .def("check_if_updated_with_experimental_data", &pa::CarbohydrateStructure::check_if_updated_with_experimental_data);

    py::class_<pa::XRayData>(m, "XRayData")
        .def(py::init<>())
        .def(py::init<std::string&, std::string&, std::string&, float, int, bool>(), py::arg("path_to_mtz_file")="undefined", py::arg("path_to_model_file")="undefined", py::arg("input_column_fobs_user")="NONE", py::arg("ipradius")=2.5, py::arg("nThreads")=-1, py::arg("debug_output")=false)
        .def("get_sugar_summary_with_experimental_data", &pa::XRayData::get_sugar_summary_with_experimental_data)
        .def("get_ligand_summary_with_experimental_data", &pa::XRayData::get_ligand_summary_with_experimental_data)
        .def("print_cpp_console_output_summary", &pa::XRayData::print_cpp_console_output_summary);

    py::class_<pa::CryoEMData>(m, "CryoEMData")
        .def(py::init<>())
        .def(py::init<std::string&, std::string&, float, float, int, bool>(), py::arg("path_to_mrc_file")="undefined", py::arg("path_to_model_file")="undefined", py::arg("resolution")="-1", py::arg("ipradius")=2.5, py::arg("nThreads")=-1, py::arg("debug_output")=false)
        .def("get_sugar_summary_with_experimental_data", &pa::CryoEMData::get_sugar_summary_with_experimental_data)
        .def("get_ligand_summary_with_experimental_data", &pa::CryoEMData::get_ligand_summary_with_experimental_data)
        .def("print_cpp_console_output_summary", &pa::CryoEMData::print_cpp_console_output_summary);
    
    py::class_<pa::OfflineGlycomicsDatabase>(m, "OfflineGlycomicsDatabase")
        .def(py::init<>())
        .def(py::init<std::string&>(), py::arg("path_to_input_file")="nopath")
        .def("import_json_file", &pa::OfflineGlycomicsDatabase::import_json_file);

    py::class_<pa::OfflineTorsionsDatabase>(m, "OfflineTorsionsDatabase")
        .def(py::init<>())
        .def(py::init<std::string&>(), py::arg("path_to_input_file")="nopath")
        .def("import_json_file", &pa::OfflineTorsionsDatabase::import_json_file);
}

///////////////////////////////////////////////// PYBIND11 BINDING DEFINITIONS END////////////////////////////////////////////////////////////////////