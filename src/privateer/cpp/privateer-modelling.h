// Library for the YSBL program Privateer (PRogramatic Identification of Various Anomalies Toothsome Entities Experience in Refinement)
// Licence: LGPL - Please check Licence.txt for details.
//
// 2013-
// York Structural Biology Laboratory
// The University of York

#ifndef PRIVATEER_MODELLING_H_INCLUDED
#define PRIVATEER_MODELLING_H_INCLUDED

#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <clipper/clipper.h>
#include <clipper/clipper-cif.h>
#include <clipper/clipper-mmdb.h>
#include <clipper/clipper-ccp4.h>
#include <clipper/clipper-contrib.h>
#include <clipper/clipper-minimol.h>
#include <clipper/contrib/sfcalc_obs.h>
#include <clipper/minimol/minimol_utils.h>
#include "clipper-glyco_data.h"
#include "clipper-glyco.h"

namespace privateer {
	namespace modelling {

		struct protein_sidechain_glycosylation
        {
            clipper::String residue_name;       
            clipper::String connected_atom;          
            clipper::String vector_point_alpha;
			clipper::String vector_point_bravo;     
            clipper::ftype Phi;        
            clipper::ftype Psi;       
        };


		struct sugar_attachment
        {
            clipper::String glycan_type;       
            clipper::String connection_atom;
			clipper::String vector_point_alpha;        
            clipper::String vector_point_bravo;   
        };

		extern const protein_sidechain_glycosylation backbone_instructions[];
        extern const sugar_attachment sugar_instructions[];
		extern const int backbone_instructions_size;
        extern const int sugar_instructions_size;

		class Grafter
    	{
      		public:
				Grafter() { } //!< null constructor
				Grafter(clipper::MiniMol receiving_model, clipper::MiniMol donor_model, bool enable_user_messages, bool debug_output);
				int get_number_of_glycans_detected() { return donor_glycans.size(); };
				clipper::MiniMol& get_receiving_model() { return receiving_model; };
				clipper::MiniMol& get_donor_model() { return donor_model; };
				std::vector<clipper::MGlycan>& get_donor_glycans() { return donor_glycans; };
				clipper::MPolymer convert_mglycan_to_mpolymer(clipper::MGlycan input);
				clipper::Coord_orth get_glycan_target_point(clipper::Coord_orth& connecting_atom, clipper::Coord_orth& vector_origin, clipper::Coord_orth& vector_target, float vectorShiftDistance);
				void graft_mpolymer_to_receiving_model(clipper::MGlycan& glycan_to_graft, clipper::MMonomer& input_protein_side_chain_residue, bool ANY_search_policy);
				clipper::Mat33<clipper::ftype> generate_rotation_matrix_from_rodrigues_rotation_formula(clipper::ftype degrees_to_rotate, clipper::Vec3<clipper::ftype> input_vector);

				int lookup_protein_backbone_glycosylation_database( clipper::String name);
				int lookup_glycan_type_glycosylation_database( clipper::String type);
			private:
				bool enable_user_messages;
				bool debug_output;
				clipper::MiniMol receiving_model;
				clipper::MiniMol donor_model;
				std::vector<clipper::MGlycan> donor_glycans;
				int numDonorGlycansDetected;

    	};


  	}
}


#endif