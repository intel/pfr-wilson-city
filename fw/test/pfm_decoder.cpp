/******************************************************************************
 * Copyright (c) 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <fstream>
#include <string>
#include <bitset>

#include "bsp_mock.h"
#include "pfm.h"
#include "pfm_utils.h"
#include "pfm_validation.h"

using namespace std;


std::ifstream::pos_type get_filesize(const std::string& filename)
{
    std::ifstream in(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

int main()
{
	std::string pfm_filename = "pfm.bin_aligned";
	alt_u8* m_raw_pfm_x86;

	cout << "PFM Decoder : " << pfm_filename << endl;

	size_t file_size_in_bytes = get_filesize(pfm_filename);
	m_raw_pfm_x86 = new alt_u8 [file_size_in_bytes];

    SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
    sys->reset();

    // Load PFM
    sys->init_x86_mem_from_file(pfm_filename, (alt_u32*) m_raw_pfm_x86);

    // Cast as PFM
    PFM* pfm = (PFM*)m_raw_pfm_x86;

	if (/*is_pfm_valid(pfm)*/1) {
		cout << "PFM is valid!" << endl;

		cout << "TAG : 0x" << std::hex << (alt_u32)pfm->tag << endl;
		cout << "SVN : 0x" << std::hex << (alt_u32)pfm->svn << endl;
		cout << "Major Rev : 0x" << std::hex << (alt_u32)pfm->major_rev << endl;
		cout << "Minor Rev : 0x" << std::hex << (alt_u32)pfm->minor_rev << endl;

		alt_u32* pfm_body_ptr = pfm->pfm_body;
		alt_u32 spi_region_num = 0;
	    // Go through the PFM Body
	    while (1)
	    {
	        alt_u8 def_type = *((alt_u8*) pfm_body_ptr);
	        if (def_type == SMBUS_RULE_DEF_TYPE)
	        {
	            PFM_SMBUS_RULE_DEF* rule_def = (PFM_SMBUS_RULE_DEF*) pfm_body_ptr;
	            if (!is_smbus_rule_valid(rule_def))
	            {
	                // TODO: Add SMBUS decoding info
	            }
	            pfm_body_ptr = incr_alt_u32_ptr(pfm_body_ptr, SMBUS_RULE_DEF_SIZE);
	        }
	        else if (def_type == SPI_REGION_DEF_TYPE)
	        {
	            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body_ptr;
	            cout << "SPI Region " << spi_region_num << endl;
	            spi_region_num++;

	            if (!is_spi_region_valid(region_def))
	            {
	                cout << "   Region hash is invalid!" << endl;
	            }

	            cout << "   Def Type: 0x" << std::hex << (alt_u32)region_def->def_type << " (" << std::bitset<32>((alt_u32)region_def->def_type) << ")" << endl;
	            cout << "   Hash Algorithm: 0x" << std::hex << (alt_u32)region_def->hash_algorithm << " (" << std::bitset<32>((alt_u32)region_def->hash_algorithm) << ")" << endl;
	            if (region_def->hash_algorithm & PFM_HASH_ALGO_SHA256_MASK)
	            {
	            	cout << "      Hash 256" << endl;
	            }
	            if (region_def->hash_algorithm & PFM_HASH_ALGO_SHA384_MASK)
	            {
	            	cout << "      Hash 384" << endl;
	            }
	            cout << "   Protection Mask: 0x" << std::hex << (alt_u32)region_def->protection_mask << " (" << std::bitset<32>((alt_u32)region_def->protection_mask) << ")" << endl;
	            if (is_spi_region_static(region_def))
	            {
	            	cout << "      Region is static" << endl;
	            }
	            else if (is_spi_region_dynamic(region_def))
	            {
	            	cout << "      Region is dynamic" << endl;
	            }

	            if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_READ_ALLOWED)
	            {
	            	cout << "      Region is read allowed" << endl;
	            }
	            else 
	            {
	            	cout << "      Region is not marked as read allowed, but is read allowed anyways" << endl;
	            }
	            
	            if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_WRITE_ALLOWED)
	            {
	            	cout << "      Region is write allowed" << endl;
	            }
	            else
	            {
	            	cout << "      Region is write protected!" << endl;
	            }

	            if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY)
	            {
	            	cout << "      Region is recovered on first recovery" << endl;
	            }
	            if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_RECOVER_ON_SECOND_RECOVERY)
	            {
	            	cout << "      Region is recovered on second recovery" << endl;
	            }
	            if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY)
	            {
	            	cout << "      Region is recovered on third recovery" << endl;
	            }

	            cout << "   Start Address: 0x" << std::hex << (alt_u32)region_def->start_addr << endl;
	            cout << "   End Address: 0x" << std::hex << (alt_u32)region_def->end_addr << endl;

	            cout << endl << endl;
	            pfm_body_ptr = get_end_of_spi_region_def(region_def);
	        }
	        else
	        {
	            // Break when there is no more region/rule definition in PFM body
	            break;
	        }
	    }


	}
	else {
		cout << "PFM is not valid!" << endl;
	}

    delete(m_raw_pfm_x86);
	return 0;
}
