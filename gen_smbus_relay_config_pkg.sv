package gen_smbus_relay_config_pkg;

    localparam NUM_RELAYS = 3;

    // Relay1 PMBus1
    localparam RELAY1_NUM_ADDRESSES = 6;
    localparam logic [6:0] RELAY1_I2C_ADDRESS1 = 7'h48;
    localparam logic [6:0] RELAY1_I2C_ADDRESS2 = 7'h56;
    localparam logic [6:0] RELAY1_I2C_ADDRESS3 = 7'h51;
    localparam logic [6:0] RELAY1_I2C_ADDRESS4 = 7'h59;
    localparam logic [6:0] RELAY1_I2C_ADDRESS5 = 7'h50;
    localparam logic [6:0] RELAY1_I2C_ADDRESS6 = 7'h58;
    localparam [RELAY1_NUM_ADDRESSES:1][6:0] RELAY1_I2C_ADDRESSES = {{7'h58}, {7'h50}, {7'h59}, {7'h51}, {7'h56}, {7'h48}};

    // Relay2 PMBus2
    localparam RELAY2_NUM_ADDRESSES = 11;
    localparam logic [6:0] RELAY2_I2C_ADDRESS1 = 7'h5a;
    localparam logic [6:0] RELAY2_I2C_ADDRESS2 = 7'h6a;
    localparam logic [6:0] RELAY2_I2C_ADDRESS3 = 7'h25;
    localparam logic [6:0] RELAY2_I2C_ADDRESS4 = 7'h26;
    localparam logic [6:0] RELAY2_I2C_ADDRESS5 = 7'h6e;
    localparam logic [6:0] RELAY2_I2C_ADDRESS6 = 7'h76;
    localparam logic [6:0] RELAY2_I2C_ADDRESS7 = 7'h70;
    localparam logic [6:0] RELAY2_I2C_ADDRESS8 = 7'h58;
    localparam logic [6:0] RELAY2_I2C_ADDRESS9 = 7'h62;
    localparam logic [6:0] RELAY2_I2C_ADDRESS10 = 7'h66;
    localparam logic [6:0] RELAY2_I2C_ADDRESS11 = 7'h72;
    localparam [RELAY2_NUM_ADDRESSES:1][6:0] RELAY2_I2C_ADDRESSES = {{7'h72}, {7'h66}, {7'h62}, {7'h58}, {7'h70}, {7'h76}, {7'h6e}, {7'h26}, {7'h25}, {7'h6a}, {7'h5a}};

    // Relay3 HSBP
    localparam RELAY3_NUM_ADDRESSES = 13;
    localparam logic [6:0] RELAY3_I2C_ADDRESS1 = 7'h4c;
    localparam logic [6:0] RELAY3_I2C_ADDRESS2 = 7'h52;
    localparam logic [6:0] RELAY3_I2C_ADDRESS3 = 7'h68;
    localparam logic [6:0] RELAY3_I2C_ADDRESS4 = 7'h6c;
    localparam logic [6:0] RELAY3_I2C_ADDRESS5 = 7'h74;
    localparam logic [6:0] RELAY3_I2C_ADDRESS6 = 7'h70;
    localparam logic [6:0] RELAY3_I2C_ADDRESS7 = 7'h53;
    localparam logic [6:0] RELAY3_I2C_ADDRESS8 = 7'h1b;
    localparam logic [6:0] RELAY3_I2C_ADDRESS9 = 7'h48;
    localparam logic [6:0] RELAY3_I2C_ADDRESS10 = 7'h50;
    localparam logic [6:0] RELAY3_I2C_ADDRESS11 = 7'h49;
    localparam logic [6:0] RELAY3_I2C_ADDRESS12 = 7'h51;
    localparam logic [6:0] RELAY3_I2C_ADDRESS13 = 7'h24;
    localparam [RELAY3_NUM_ADDRESSES:1][6:0] RELAY3_I2C_ADDRESSES = {{7'h24}, {7'h51}, {7'h49}, {7'h50}, {7'h48}, {7'h1b}, {7'h53}, {7'h70}, {7'h74}, {7'h6c}, {7'h68}, {7'h52}, {7'h4c}};

endpackage
