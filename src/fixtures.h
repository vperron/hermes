/**
 * =====================================================================================
 *
 *   @file fixtures.h
 *
 *   
 *        Version:  1.0
 *        Created:  04/22/2013 02:28:02 PM
 *
 *
 *   @section DESCRIPTION
 *
 *       iContains a few test fixtures for various tests.
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

#ifndef _HERMES_FIXTURES_H_
#define _HERMES_FIXTURES_H_

/*  Json to be structured  */
#define Fix_ts "2013-04-01T11:11:11.048+00:11"
#define Fix_v1 "00edcabe"
#define Fix_v2 "50ccf8"
#define Fix_v3 "-42"
#define Fix_v4 "beefdead"
#define Fix_v5 "foo\\\""
#define Fix_json_built_0 "{\"ts\":\"2013-04-01T11:11:11.048+00:11\",\"mac\":\"00edcabe\",\"manuf\":\"50ccf8\",\"rssi\":-42,\"bssid\":\"beefdead\",\"ssid\":\"foo\\\"\"}"
#define Fix_json_built_1 "{\"ts\":\"2013-04-01T11:11:11.048+00:11\",\"mac\":\"00edcabe\",\"manuf\":\"50ccf8\",\"rssi\":-42,\"bssid\":\"beefdead\",\"ssid\":\"\"}"
#define Fix_json_built_2 "{\"ts\":\"2013-04-01T11:11:11.048+00:11\",\"mac\":\"d34db4b3\",\"manuf\":\"50ccf8\",\"rssi\":-53,\"bssid\":\"beefdead\",\"ssid\":\"\"}"
#define Fix_json_hash_0 "00edcabe50ccf8foo"
#define Fix_json_hash_1 "00edcabe50ccf8"
#define Fix_json_hash_2 "d34db4b350ccf8"

/*  Json with escaping  */
#define Fix_v5_escapes "790160156250573\"ab\\uu67\"hh"
#define Fix_json_built_0_escapes "{\"ts\":\"2013-04-01T11:11:11.048+00:11\",\"mac\":\"00edcabe\",\"manuf\":\"50ccf8\",\"rssi\":-42,\"bssid\":\"beefdead\",\"ssid\":\"790160156250573\"ab\\uu67\"hh\"}"

#define Fix_v5_other_escapes "790160156250573\"ab\\uu67\"hh\\\\"
#define Fix_json_built_0_other_escapes "{\"ts\":\"2013-04-01T11:11:11.048+00:11\",\"mac\":\"00edcabe\",\"manuf\":\"50ccf8\",\"rssi\":-42,\"bssid\":\"beefdead\",\"ssid\":\"790160156250573\"ab\\uu67\"hh\\\\\"}"


/*  Json to be aggregated  */
#define Fix_hash1 "abcdef00"
#define Fix_json1 "{\"ts\":\"2013-04-01T11:11:11.048+00:11\",\"mac\":\"00edcabe\",\"manuf\":\"50ccf8\",\"rssi\":-1,\"bssid\":\"beefdead\",\"ssid\":\"A\"}"
#define Fix_json2 "{\"ts\":\"2013-04-01T22:22:22.048+00:22\",\"mac\":\"00edcabe\",\"manuf\":\"50ccf8\",\"rssi\":-2,\"bssid\":\"beefdead\",\"ssid\":\"B\"}"
#define Fix_json3 "{\"ts\":\"2013-04-01T33:33:33.048+00:33\",\"mac\":\"00edcabe\",\"manuf\":\"50ccf8\",\"rssi\":-3,\"bssid\":\"beefdead\",\"ssid\":\"C\"}"
#define Fix_json_aggreg "[" Fix_json1 "," Fix_json2 "," Fix_json3 "]"

#define Fix_json_aggreg_1 "[" Fix_json3 "," Fix_json2 "," Fix_json3 "," Fix_json2 "]"
#define Fix_json_aggreg_2 "[" Fix_json3 "," Fix_json2 "," Fix_json2 "," Fix_json1 "]"
#define Fix_json_aggreg_3 "[" Fix_json1 "," Fix_json3 "," Fix_json1 "," Fix_json2 "," Fix_json1 "]"

/*  Fake pointer values  */
#define FAKE_PTR_VAL_0 1337
#define FAKE_PTR_VAL_1 1338
#define FAKE_PTR_VAL_2 1339

/*  Other values  */
#define Fix_persistpath "foopersist"
#define Fix_memorypath "foomem"
#define Fix_file_id_0 "1234567-b87e4edf"
#define Fix_file_name_0 "foodirectory/1234567-b87e4edf.dat.gz"
#define Fix_file_id_1 "2345679-54VTVGgv"
#define Fix_file_name_1 "foodirectory/2345679-54VTVGgv.dat.gz"
#define Fix_file_id_2 "345687654-dfg45678k"
#define Fix_file_name_2 "foodirectory/345687654-dfg45678k.dat.gz"

#endif // _HERMES_FIXTURES_H_
