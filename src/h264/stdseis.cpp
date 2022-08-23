#include "stdseis.h"

namespace metamix::h264 {

Metadata<SeiKind>
build_empty_metadata(InputId input_id, ClockTS ts)
{
  // clang-format off
  static auto EMPTY_SEI = std::make_shared<OwnedSeiPayload>(SeiType::USER_DATA_REGISTERED, std::vector<uint8_t>{
    // == H.264/H.265 SEI prefix ==
    181,                                              // itu_t_t35_country_code -> USA
    0, 49,                                            // itu_t_t35_provider_code -> ATSC_user_data
    'G', 'A', '9', '4',                               // ATSC_user_identifier
    3,                                                // ATSC1_data_user_data_type_code -> DTVCC

    // == user_data_type_structure ==
    0b010'00000 /* flags */ | 4 /* cc_count */,       // flags = [process_em_data, process_cc_data, additional_data]
    0x00,                                             // em_data, 0 because process_em_data == 0

    // == cc_data_pkt's, mind parity bits for 608 data! ==
    0b11111'1'00, 0x80, 0x80,                         // NTSC_CC_FIELD_1: XDS NULL PADDING
    0b11111'1'01, 0x01, 0x85,                         // NTSC_CC_FIELD_2: XDS CLASS + TYPE

    0b11111'0'10, 0x00, 0x00,                         // First invalid packet of type DTVCC_PACKET_DATA marks end of
    0b11111'0'10, 0x00, 0x00,                         // DTVCC packet. Rest is interpreted as padding.

    // == user_data_type_structure cont. ==
    0xFF,                                             // marker_bits

    // No ATSC_reserved_user_data because additional_data == 0
  });
  // clang-format on

  return { input_id, ts, ts, std::numeric_limits<int>::max(), EMPTY_SEI };
}

Metadata<SeiKind>
build_cc_reset_metadata(InputId input_id, ClockTS ts)
{
  // clang-format off
  static auto RESET_SEI = std::make_shared<OwnedSeiPayload>(SeiType::USER_DATA_REGISTERED, std::vector<uint8_t>{
    // == H.264/H.265 SEI prefix ==
    181,                                              // itu_t_t35_country_code -> USA
    0, 49,                                            // itu_t_t35_provider_code -> ATSC_user_data
    'G', 'A', '9', '4',                               // ATSC_user_identifier
    3,                                                // ATSC1_data_user_data_type_code -> DTVCC

    // == user_data_type_structure ==
    0b010'00000 /* flags */ | 18 /* cc_count */,      // flags = [process_em_data, process_cc_data, additional_data]
    0x00,                                             // em_data, 0 because process_em_data == 0

    // == cc_data_pkt's, mind parity bits for 608 data! ==
    // 608 reset
    0b11111'1'00, 0x94, 0x2C,                         // NTSC_CC_FIELD_1: Data Channel 1, Erase Displayed Memory
    0b11111'1'00, 0x94, 0xAE,                         // NTSC_CC_FIELD_1: Data Channel 1, Erase Non-Displayed Memory
    0b11111'1'00, 0x94, 0x2F,                         // NTSC_CC_FIELD_1: Data Channel 1, End of Caption

    0b11111'1'00, 0x1C, 0x2C,                         // NTSC_CC_FIELD_1: Data Channel 2, Erase Displayed Memory
    0b11111'1'00, 0x1C, 0xAE,                         // NTSC_CC_FIELD_1: Data Channel 2, Erase Non-Displayed Memory
    0b11111'1'00, 0x1C, 0x2F,                         // NTSC_CC_FIELD_1: Data Channel 2, End of Caption

    0b11111'1'01, 0x94, 0x2C,                         // NTSC_CC_FIELD_2: Data Channel 1, Erase Displayed Memory
    0b11111'1'01, 0x94, 0xAE,                         // NTSC_CC_FIELD_2: Data Channel 1, Erase Non-Displayed Memory
    0b11111'1'01, 0x94, 0x2F,                         // NTSC_CC_FIELD_2: Data Channel 1, End of Caption

    0b11111'1'01, 0x1C, 0x2C,                         // NTSC_CC_FIELD_2: Data Channel 2, Erase Displayed Memory
    0b11111'1'01, 0x1C, 0xAE,                         // NTSC_CC_FIELD_2: Data Channel 2, Erase Non-Displayed Memory
    0b11111'1'01, 0x1C, 0x2F,                         // NTSC_CC_FIELD_2: Data Channel 2, End of Caption

    // 708 reset
    0b11111'1'11, 0x02, 0x21,                         // DTVCC_PACKET_START: Headers
    0b11111'1'10, 0x8F, 0x00,                         // DTVCC_PACKET_DATA: Reset Primary Language Service
    0b11111'1'11, 0x02, 0x41,                         // DTVCC_PACKET_START: Headers
    0b11111'1'10, 0x8F, 0x00,                         // DTVCC_PACKET_DATA: Reset Secondary Language Service

    0b11111'0'10, 0x00, 0x00,                         // End of DTVCC packet.
    0b11111'0'10, 0x00, 0x00,                         //

    // == user_data_type_structure cont. ==
    0xFF,                                             // marker_bits

    // No ATSC_reserved_user_data because additional_data == 0
  });
  // clang-format on

  return { input_id, ts, ts, std::numeric_limits<int>::min(), RESET_SEI };
}
}
