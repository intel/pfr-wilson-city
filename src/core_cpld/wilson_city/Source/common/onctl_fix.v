//////////////////////////////////////////////////////////////////////////////////
/*!
 \brief    <b>%ONCTL logic latch for fix ONCTL_N logic issue during power button override </b>
 \file     onctl_fix.v
 \details    <b>Image of the Block:</b>
    \image html
     <b>Description:</b> \n
    ONCTL logic latch for fix ONCTL_N logic issue during power button override\n\n
 \brief  <b>Last modified</b>
   $Date:   Nov 22, 2016 $
   $Author:  paul.xu@intel.com $
   Project   : BNP
   Group   : BD
 \version
    0.9 \b  paul.xu@intel.com - File creation\n
	1.0 \b  ricardo.ramos.contreras@intel.com - Replicated expected behavior\n
 \copyright Intel Proprietary -- Copyright 2016 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module onctl_fix (
    input iClk_2M,
    input iRst_n,

    input FM_BMC_ONCTL_N,
    input FM_SLPS3_N,
	input FM_SLPS4_N,

    output  FM_BMC_ONCTL_N_LATCH
);

reg rFM_BMC_ONCTL_N_LATCH;

always @ (posedge iClk_2M) begin
    if (!iRst_n) begin
        rFM_BMC_ONCTL_N_LATCH <= 1'b1;
    end
    else begin
		case({FM_SLPS3_N, FM_SLPS4_N, FM_BMC_ONCTL_N})
			3'b110:
				rFM_BMC_ONCTL_N_LATCH <= 1'b0;
			3'b001:
				rFM_BMC_ONCTL_N_LATCH <= 1'b1;
			3'b011:
				rFM_BMC_ONCTL_N_LATCH <= 1'b1;
			3'b101:
				rFM_BMC_ONCTL_N_LATCH <= 1'b1;
			default:
				rFM_BMC_ONCTL_N_LATCH <= rFM_BMC_ONCTL_N_LATCH;
		endcase
    end
end

assign FM_BMC_ONCTL_N_LATCH = rFM_BMC_ONCTL_N_LATCH;

endmodule
