add_executable(fpga_unit_tests core_cli_tests.cpp ../src/cli/command_line.cpp)
target_link_libraries(fpga_unit_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME core_and_cli_unit COMMAND fpga_unit_tests)

find_program(FPGA_PYTHON3_EXECUTABLE NAMES python3)
if(FPGA_PYTHON3_EXECUTABLE)
    add_test(NAME rule_audit_filename_environment_unit
        COMMAND "${FPGA_PYTHON3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/audit_t_linter_rules_tests.py")
endif()

add_executable(fpga_design_database_tests design_database_tests.cpp)
target_link_libraries(fpga_design_database_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME design_database_unit COMMAND fpga_design_database_tests)

add_executable(fpga_analyzer_tests analyzer_tests.cpp)
target_link_libraries(fpga_analyzer_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME analyzer_unit COMMAND fpga_analyzer_tests)

add_executable(fpga_config_file_tests config_file_tests.cpp)
target_link_libraries(fpga_config_file_tests PRIVATE fpga_core fpga_build_options)
target_compile_definitions(fpga_config_file_tests PRIVATE
    FPGA_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME config_file_unit COMMAND fpga_config_file_tests)

add_executable(fpga_baseline_tests baseline_tests.cpp)
target_link_libraries(fpga_baseline_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME baseline_unit COMMAND fpga_baseline_tests)

add_executable(fpga_rule_catalog_tests rule_catalog_tests.cpp)
target_link_libraries(fpga_rule_catalog_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME rule_catalog_unit COMMAND fpga_rule_catalog_tests)

add_executable(fpga_standard_accuracy_tests standard_accuracy_tests.cpp)
target_link_libraries(fpga_standard_accuracy_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME standard_accuracy_unit COMMAND fpga_standard_accuracy_tests)
add_executable(fpga_constant_expression_tests verilog_constant_expression_tests.cpp)
target_link_libraries(fpga_constant_expression_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME constant_expression_unit COMMAND fpga_constant_expression_tests)
add_executable(fpga_native_integer_tests native_verilog_integer_tests.cpp)
target_link_libraries(fpga_native_integer_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME native_integer_fold_unit COMMAND fpga_native_integer_tests)
add_executable(fpga_function_checks_tests verilog_function_checks_tests.cpp)
target_link_libraries(fpga_function_checks_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME function_checks_unit COMMAND fpga_function_checks_tests)
add_executable(fpga_unknown_arithmetic_tests unknown_arithmetic_tests.cpp)
target_link_libraries(fpga_unknown_arithmetic_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME unknown_arithmetic_unit COMMAND fpga_unknown_arithmetic_tests)
add_executable(fpga_default_assignment_tests default_assignment_unknown_tests.cpp)
target_link_libraries(fpga_default_assignment_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME default_assignment_unit COMMAND fpga_default_assignment_tests)
add_executable(fpga_case_label_tests verilog_case_labels_tests.cpp)
target_link_libraries(fpga_case_label_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME case_label_unit COMMAND fpga_case_label_tests)
add_executable(fpga_multiple_reset_tests verilog_multiple_reset_tests.cpp)
target_link_libraries(fpga_multiple_reset_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME multiple_reset_unit COMMAND fpga_multiple_reset_tests)
add_executable(fpga_repeat_checks_tests verilog_repeat_checks_tests.cpp)
target_link_libraries(fpga_repeat_checks_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME repeat_checks_unit COMMAND fpga_repeat_checks_tests)
add_executable(fpga_function_recursion_tests function_recursion_tests.cpp)
target_link_libraries(fpga_function_recursion_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME function_recursion_unit COMMAND fpga_function_recursion_tests)
add_executable(fpga_index_checks_tests verilog_index_checks_tests.cpp)
target_link_libraries(fpga_index_checks_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME index_checks_unit COMMAND fpga_index_checks_tests)
add_executable(fpga_naming_style_tests verilog_naming_style_tests.cpp)
target_link_libraries(fpga_naming_style_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME naming_style_predicate_unit COMMAND fpga_naming_style_tests)
add_executable(fpga_naming_declarations_tests verilog_naming_declarations_tests.cpp)
target_link_libraries(fpga_naming_declarations_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME naming_declarations_unit COMMAND fpga_naming_declarations_tests)
add_executable(fpga_naming_shape_tests verilog_naming_shape_tests.cpp)
target_link_libraries(fpga_naming_shape_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME naming_shape_unit COMMAND fpga_naming_shape_tests)
add_executable(fpga_name_length_tests verilog_name_length_tests.cpp)
target_link_libraries(fpga_name_length_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME name_length_unit COMMAND fpga_name_length_tests)
add_executable(fpga_power_names_tests verilog_power_names_tests.cpp)
target_link_libraries(fpga_power_names_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME power_names_unit COMMAND fpga_power_names_tests)
add_executable(fpga_vhdl_edge_condition_tests vhdl_edge_condition_tests.cpp)
target_link_libraries(fpga_vhdl_edge_condition_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME vhdl_edge_condition_unit COMMAND fpga_vhdl_edge_condition_tests)
add_executable(fpga_vhdl_clock_control_tests vhdl_clock_control_tests.cpp)
target_link_libraries(fpga_vhdl_clock_control_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME vhdl_clock_control_unit COMMAND fpga_vhdl_clock_control_tests)
add_executable(fpga_vhdl_fsm_model_tests vhdl_fsm_model_tests.cpp)
target_link_libraries(fpga_vhdl_fsm_model_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME vhdl_fsm_model_unit COMMAND fpga_vhdl_fsm_model_tests)
add_executable(fpga_verilog_header_policy_tests verilog_header_policy_tests.cpp)
target_link_libraries(fpga_verilog_header_policy_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME verilog_header_policy_unit COMMAND fpga_verilog_header_policy_tests)
add_executable(fpga_verilog_keyword_names_tests verilog_keyword_names_tests.cpp)
target_link_libraries(fpga_verilog_keyword_names_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME verilog_keyword_names_unit COMMAND fpga_verilog_keyword_names_tests)
add_test(NAME cli_vhdl_native_clock_control COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB9765-2020 "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_native_clock_control.vhd")
set_tests_properties(cli_vhdl_native_clock_control PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"A-10-4\",\"state\":\"violated\"")

add_executable(fpga_line_statistics_tests line_statistics_tests.cpp)
target_link_libraries(fpga_line_statistics_tests PRIVATE fpga_core fpga_build_options)
add_test(NAME line_statistics_unit COMMAND fpga_line_statistics_tests)
add_test(NAME cli_line_count COMMAND fpga_analyzer_cli --line-count --format json
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/malformed.v")
set_tests_properties(cli_line_count PROPERTIES
    PASS_REGULAR_EXPRESSION "\"lineStatistics\".*\"filesAnalyzed\":1.*\"violations\":0")

add_test(NAME cli_help COMMAND fpga_analyzer_cli --help)
set_tests_properties(cli_help PROPERTIES PASS_REGULAR_EXPRESSION "用法：fpga_analyzer_cli")
add_test(NAME cli_version COMMAND fpga_analyzer_cli --version)
set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")
add_test(NAME cli_localparam_index_bounds COMMAND fpga_analyzer_cli --standard GJB10157-2021
    --format json --fail-on none "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/localparam_index.sv")
set_tests_properties(cli_localparam_index_bounds PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"R-13-8\",\"severity\":")
add_test(NAME cli_sensitivity_implicit_names COMMAND fpga_analyzer_cli --standard GJB10157-2021
    --format json --fail-on none "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sensitivity_implicit.v")
set_tests_properties(cli_sensitivity_implicit_names PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"R-13-9\".*组合always敏感列表中信号B_COND是冗余的")
add_test(NAME cli_native_keywords_explicit COMMAND fpga_analyzer_cli --format json --fail-on none
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/keyword_policy_explicit.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/verilog_native_keyword_names.sv")
set_tests_properties(cli_native_keywords_explicit PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"R-2-3\",\"count\":4")
add_test(NAME cli_native_keywords_disabled COMMAND fpga_analyzer_cli --format json --fail-on none
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/keyword_policy_disabled.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/verilog_native_keyword_names.sv")
set_tests_properties(cli_native_keywords_disabled PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"R-2-3\",\"state\":\"manual-review-required\"")
add_test(NAME cli_native_keywords_unspecified COMMAND fpga_analyzer_cli --format json --fail-on none
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/keyword_policy_unspecified.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/verilog_native_keyword_names.sv")
set_tests_properties(cli_native_keywords_unspecified PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"R-2-3\",\"count\":3.*\"origin\":\"documented-standard-default\".*\"ruleId\":\"R-2-3\",\"state\":\"violated\"")
add_test(NAME cli_native_header_explicit COMMAND fpga_analyzer_cli --format json --fail-on none
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/header_policy_explicit.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/verilog_native_header.sv")
set_tests_properties(cli_native_header_explicit PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"A-1-1\",\"count\":1.*\"origin\":\"explicit-project\"")
add_test(NAME cli_native_header_disabled COMMAND fpga_analyzer_cli --format json --fail-on none
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/header_policy_disabled.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/verilog_native_header.sv")
set_tests_properties(cli_native_header_disabled PROPERTIES
    PASS_REGULAR_EXPRESSION "\"origin\":\"disabled\",\"requirements\":\\[\\].*\"ruleId\":\"A-1-1\",\"state\":\"manual-review-required\"")
add_test(NAME cli_native_header_template COMMAND fpga_analyzer_cli --format json --fail-on none
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/header_policy_template.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/verilog_native_header.sv")
set_tests_properties(cli_native_header_template PROPERTIES
    PASS_REGULAR_EXPRESSION "\"origin\":\"product-template\"")
add_test(NAME cli_native_power_names COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB10157-2021 "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/verilog_native_power_names.sv")
set_tests_properties(cli_native_power_names PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"R-2-4\",\"count\":3")
add_test(NAME cli_power_on_initial_accepted COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB10157-2021 --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/power_on_non_sram.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/power_on_register.v")
set_tests_properties(cli_power_on_initial_accepted PROPERTIES
    PASS_REGULAR_EXPRESSION "\"technology\":\"non-sram\",\"acceptInitialBlocks\":true.*\"ruleId\":\"R-5-5\",\"state\":\"manual-review-required\"")
add_test(NAME cli_power_on_initial_ignored COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB10157-2021 --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/power_on_non_sram.conf"
    --initial-blocks ignore "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/power_on_register.v")
set_tests_properties(cli_power_on_initial_ignored PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"R-5-5\",\"count\":1.*\"technology\":\"non-sram\",\"acceptInitialBlocks\":false")
add_test(NAME cli_power_on_unknown_technology COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB10157-2021 --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/power_on_non_sram.conf"
    --fpga-technology unspecified "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/power_on_register.v")
set_tests_properties(cli_power_on_unknown_technology PROPERTIES
    PASS_REGULAR_EXPRESSION "\"technology\":\"unspecified\".*\"ruleId\":\"R-5-5\",\"state\":\"manual-review-required\"")
add_test(NAME cli_fsm_area_priority COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB9765-2020 --fsm-priority area "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_native_fsm.vhd")
set_tests_properties(cli_fsm_area_priority PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"A-11-4\",\"state\":\"violated\"")
add_test(NAME cli_fsm_speed_priority COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB9765-2020 --fsm-priority speed "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_native_fsm.vhd")
set_tests_properties(cli_fsm_speed_priority PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"A-11-5\",\"state\":\"manual-review-required\"")
add_test(NAME cli_fsm_unspecified_priority COMMAND fpga_analyzer_cli --format json --fail-on none
    --standard GJB9765-2020 "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_native_fsm.vhd")
set_tests_properties(cli_fsm_unspecified_priority PROPERTIES
    PASS_REGULAR_EXPRESSION "\"ruleId\":\"A-11-4\",\"state\":\"violated\"")
add_test(NAME cli_analyze_json COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/risky.sv"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/risky.vhd")
set_tests_properties(cli_analyze_json PROPERTIES
    PASS_REGULAR_EXPRESSION "\"filesAnalyzed\":2.*\"designUnits\":2")
add_test(NAME cli_project_config COMMAND fpga_analyzer_cli --format json
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/project.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/config_project")
set_tests_properties(cli_project_config PROPERTIES
    PASS_REGULAR_EXPRESSION "\"filesAnalyzed\":1.*\"external\":true")
add_test(NAME cli_sarif COMMAND fpga_analyzer_cli --format sarif --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/risky.sv")
set_tests_properties(cli_sarif PROPERTIES
    PASS_REGULAR_EXPRESSION "\"version\":\"2.1.0\".*\"physicalLocation\"")
add_test(NAME cli_html COMMAND fpga_analyzer_cli --format html --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/risky.vhd")
set_tests_properties(cli_html PROPERTIES
    PASS_REGULAR_EXPRESSION "FPGA HDL 静态分析报告")
add_test(NAME cli_malformed_source COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/malformed.v")
set_tests_properties(cli_malformed_source PROPERTIES
    PASS_REGULAR_EXPRESSION "SYNTAX-UNCLOSED-CONSTRUCT.*SYNTAX-UNCLOSED-DELIMITER")
add_test(NAME cli_preprocessor_config COMMAND fpga_analyzer_cli --format json --fail-on none
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/project.conf"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/preprocessor/top.sv")
set_tests_properties(cli_preprocessor_config PROPERTIES
    PASS_REGULAR_EXPRESSION "\"name\":\"feature_top\"")
add_test(NAME cli_unconnected_named_inputs COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unconnected_inputs.sv")
set_tests_properties(cli_unconnected_named_inputs PROPERTIES
    PASS_REGULAR_EXPRESSION "STRUCT-UNCONNECTED-INPUT-PORT")
add_test(NAME cli_named_parameters COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/named_parameters.sv")
set_tests_properties(cli_named_parameters PROPERTIES
    PASS_REGULAR_EXPRESSION "STRUCT-DUPLICATE-NAMED-PARAMETER.*STRUCT-UNKNOWN-NAMED-PARAMETER")
add_test(NAME cli_required_parameters COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/required_parameters.sv")
set_tests_properties(cli_required_parameters PROPERTIES
    PASS_REGULAR_EXPRESSION "STRUCT-MISSING-REQUIRED-PARAMETER.*STRUCT-POSITIONAL-PARAMETER-COUNT.*STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE")
add_test(NAME cli_positional_inputs COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/positional_inputs.sv")
set_tests_properties(cli_positional_inputs PROPERTIES
    PASS_REGULAR_EXPRESSION "STRUCT-UNCONNECTED-INPUT-PORT.*STRUCT-POSITIONAL-PORT-COUNT")
add_test(NAME cli_vhdl_association_order COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/association_order.vhd")
set_tests_properties(cli_vhdl_association_order PROPERTIES
    PASS_REGULAR_EXPRESSION "STRUCT-DUPLICATE-NAMED-PARAMETER.*STRUCT-DUPLICATE-NAMED-PORT.*STRUCT-VHDL-ASSOCIATION-ORDER")
add_test(NAME cli_duplicate_declarations COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_declarations.sv")
set_tests_properties(cli_duplicate_declarations PROPERTIES
    PASS_REGULAR_EXPRESSION "STRUCT-DUPLICATE-PARAMETER-DECLARATION.*STRUCT-DUPLICATE-PORT-DECLARATION")
add_test(NAME cli_summary_json COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_declarations.sv")
set_tests_properties(cli_summary_json PROPERTIES
    PASS_REGULAR_EXPRESSION "\"severityCounts\".*\"critical\":0.*\"error\":2.*\"ruleCounts\"")
add_test(NAME cli_summary_html COMMAND fpga_analyzer_cli --format html --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_declarations.sv")
set_tests_properties(cli_summary_html PROPERTIES
    PASS_REGULAR_EXPRESSION "规则汇总.*STRUCT-DUPLICATE-PORT-DECLARATION")
add_test(NAME cli_unused_input_port COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unused_inputs.sv")
set_tests_properties(cli_unused_input_port PROPERTIES
    PASS_REGULAR_EXPRESSION "RTL-UNUSED-INPUT-PORT.*\"signal\":\"unused\"")
add_test(NAME cli_duplicate_signals COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_signals.sv")
set_tests_properties(cli_duplicate_signals PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":3.*STRUCT-DUPLICATE-SIGNAL-DECLARATION")
add_test(NAME cli_unconnected_outputs COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unconnected_outputs.sv")
set_tests_properties(cli_unconnected_outputs PROPERTIES
    PASS_REGULAR_EXPRESSION "STRUCT-UNCONNECTED-OUTPUT-PORT.*\"count\":4")
add_test(NAME cli_file_list COMMAND fpga_analyzer_cli --format json --fail-on none
    --file-list "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/filelists/root.f")
set_tests_properties(cli_file_list PROPERTIES
    PASS_REGULAR_EXPRESSION "\"filesAnalyzed\":2.*\"designUnits\":2")
add_test(NAME cli_file_list_config COMMAND fpga_analyzer_cli --format json
    --config "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/file_list.conf")
set_tests_properties(cli_file_list_config PROPERTIES
    PASS_REGULAR_EXPRESSION "\"filesAnalyzed\":2.*\"designUnits\":2")
add_test(NAME cli_vhdl_process_no_suspension COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_process_no_suspension.vhd")
set_tests_properties(cli_vhdl_process_no_suspension PROPERTIES
    PASS_REGULAR_EXPRESSION "VHDL-PROCESS-NO-SUSPENSION.*\"severity\":\"error\"")
add_test(NAME cli_vhdl_wait_with_sensitivity COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_wait_with_sensitivity.vhd")
set_tests_properties(cli_vhdl_wait_with_sensitivity PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*VHDL-WAIT-WITH-SENSITIVITY")
add_test(NAME cli_systemverilog_process_controls COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/systemverilog_process_controls.sv")
set_tests_properties(cli_systemverilog_process_controls PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":5.*SV-ALWAYS-COMB-EVENT-CONTROL.*SV-ALWAYS-FF-EVENT-CONTROL.*SV-ALWAYS-LATCH-EVENT-CONTROL.*SV-ALWAYS-LATCH-NO-LATCH")
add_test(NAME cli_redundant_sensitivity COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/redundant_sensitivity.sv")
set_tests_properties(cli_redundant_sensitivity PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-REDUNDANT-SENSITIVITY.*\"severity\":\"info\"")
add_test(NAME cli_mixed_assignment_style COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/mixed_assignment_style.sv")
set_tests_properties(cli_mixed_assignment_style PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*VL-MIXED-ASSIGNMENT-STYLE.*\"severity\":\"warning\"")
add_test(NAME cli_tokenized_assignment_styles COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tokenized_assignment_styles.sv")
set_tests_properties(cli_tokenized_assignment_styles PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*VL-COMB-NONBLOCKING.*VL-SEQ-BLOCKING")
add_test(NAME cli_tokenized_cases COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tokenized_cases.sv")
set_tests_properties(cli_tokenized_cases PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*VL-CASE-DEFAULT.*VL-CASEX")
add_test(NAME cli_gjb10157_defparam COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gjb10157_defparam.sv")
set_tests_properties(cli_gjb10157_defparam PROPERTIES
    PASS_REGULAR_EXPRESSION "VL-DEFPARAM.*\"severity\":\"warning\"")
add_test(NAME cli_standard_gjb10157 COMMAND fpga_analyzer_cli --standard gjb10157
    --format json --fail-on none "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gjb10157_defparam.sv")
set_tests_properties(cli_standard_gjb10157 PROPERTIES
    PASS_REGULAR_EXPRESSION "\"id\":\"GJB10157-2021\",\"assessments\".*\"ruleId\":\"A-2-6\"")
add_test(NAME cli_gjb9765_case_buffer COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gjb9765_case_buffer.vhd")
set_tests_properties(cli_gjb9765_case_buffer PROPERTIES
    PASS_REGULAR_EXPRESSION "VHDL-BUFFER-PORT.*VHDL-CASE-OTHERS")
add_test(NAME cli_standard_gjb9765 COMMAND fpga_analyzer_cli --standard gjb9765
    --format json --fail-on none "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gjb9765_case_buffer.vhd")
set_tests_properties(cli_standard_gjb9765 PROPERTIES
    PASS_REGULAR_EXPRESSION "\"id\":\"GJB9765-2020\".*\"ruleId\":\"R-11-1\".*\"ruleId\":\"R-12-3\"")
add_test(NAME cli_describe_gjb_rule COMMAND fpga_analyzer_cli
    --describe-rule GJB10157.9.2.2)
set_tests_properties(cli_describe_gjb_rule PROPERTIES
    PASS_REGULAR_EXPRESSION "判定方式：automated.*正确案例判据.*违反案例判据")
add_test(NAME cli_vhdl_multiline_wait COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_multiline_wait.vhd")
set_tests_properties(cli_vhdl_multiline_wait PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*VHDL-WAIT.*\"line\":6")
add_test(NAME cli_tokenized_vhdl_keywords COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tokenized_vhdl_keywords.vhd")
set_tests_properties(cli_tokenized_vhdl_keywords PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*VHDL-NONSTD-NUMERIC.*VHDL-SHARED-VARIABLE")
add_test(NAME cli_constant_condition COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constant_condition.sv")
set_tests_properties(cli_constant_condition PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-CONSTANT-CONDITION.*\"severity\":\"warning\"")
add_test(NAME cli_vhdl_constant_condition COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constant_condition.vhd")
set_tests_properties(cli_vhdl_constant_condition PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*RTL-CONSTANT-CONDITION.*\"count\":2")
add_test(NAME cli_duplicate_sensitivity COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_sensitivity.sv")
set_tests_properties(cli_duplicate_sensitivity PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-DUPLICATE-SENSITIVITY.*\"severity\":\"info\"")
add_test(NAME cli_vhdl_duplicate_sensitivity COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_sensitivity.vhd")
set_tests_properties(cli_vhdl_duplicate_sensitivity PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-DUPLICATE-SENSITIVITY.*\"severity\":\"info\"")
add_test(NAME cli_self_assignment COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/self_assignment.sv")
set_tests_properties(cli_self_assignment PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-SELF-ASSIGNMENT.*\"severity\":\"info\"")
add_test(NAME cli_vhdl_self_assignment COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/self_assignment.vhd")
set_tests_properties(cli_vhdl_self_assignment PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-SELF-ASSIGNMENT.*\"severity\":\"info\"")
add_test(NAME cli_empty_process COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/empty_process.sv")
set_tests_properties(cli_empty_process PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-EMPTY-PROCESS.*\"severity\":\"info\"")
add_test(NAME cli_vhdl_empty_process COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/empty_process.vhd")
set_tests_properties(cli_vhdl_empty_process PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-EMPTY-PROCESS.*\"severity\":\"info\"")
add_test(NAME cli_unused_parameter COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unused_parameter.sv")
set_tests_properties(cli_unused_parameter PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-UNUSED-PARAMETER.*\"severity\":\"info\"")
add_test(NAME cli_vhdl_unused_parameter COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unused_parameter.vhd")
set_tests_properties(cli_vhdl_unused_parameter PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-UNUSED-PARAMETER.*\"severity\":\"info\"")
add_test(NAME cli_vhdl_split_unused_parameter COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_generic_split")
set_tests_properties(cli_vhdl_split_unused_parameter PROPERTIES
    PASS_REGULAR_EXPRESSION "\"filesAnalyzed\":2.*\"violations\":1.*RTL-UNUSED-PARAMETER.*\"severity\":\"info\"")
add_test(NAME cli_async_reset_polarity_mismatch COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/async_reset_polarity_mismatch.sv")
set_tests_properties(cli_async_reset_polarity_mismatch PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-ASYNC-RESET-POLARITY-MISMATCH.*\"severity\":\"error\"")
add_test(NAME cli_async_reset_not_first COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/async_reset_not_first.sv")
set_tests_properties(cli_async_reset_not_first PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-ASYNC-RESET-NOT-FIRST.*\"severity\":\"error\"")
add_test(NAME cli_vhdl_async_reset_not_first COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/async_reset_not_first.vhd")
set_tests_properties(cli_vhdl_async_reset_not_first PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-ASYNC-RESET-NOT-FIRST.*\"severity\":\"error\"")
add_test(NAME cli_async_reset_unguarded_logic COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/async_reset_unguarded_logic.sv")
set_tests_properties(cli_async_reset_unguarded_logic PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-ASYNC-RESET-UNGUARDED-LOGIC.*\"severity\":\"error\"")
add_test(NAME cli_vhdl_async_reset_unguarded_logic COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/async_reset_unguarded_logic.vhd")
set_tests_properties(cli_vhdl_async_reset_unguarded_logic PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-ASYNC-RESET-UNGUARDED-LOGIC.*\"severity\":\"error\"")
add_test(NAME cli_complex_edge_event COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/complex_edge_event.sv")
set_tests_properties(cli_complex_edge_event PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-COMPLEX-EDGE-EVENT.*\"severity\":\"warning\"")
add_test(NAME cli_vhdl_complex_edge_event COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/complex_edge_event.vhd")
set_tests_properties(cli_vhdl_complex_edge_event PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-COMPLEX-EDGE-EVENT.*\"severity\":\"warning\"")
add_test(NAME cli_legacy_vhdl_edge COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/legacy_vhdl_edge.vhd")
set_tests_properties(cli_legacy_vhdl_edge PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*\"clocks\":2.*\"sequentialProcesses\":2.*CDC-DIRECT-CROSSING.*\"severity\":\"warning\"")
add_test(NAME cli_always_ff_nonedge_event COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/always_ff_nonedge_event.sv")
set_tests_properties(cli_always_ff_nonedge_event PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*SV-ALWAYS-FF-NONEDGE-EVENT.*\"severity\":\"error\"")
add_test(NAME cli_duplicate_condition COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_condition.sv")
set_tests_properties(cli_duplicate_condition PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-DUPLICATE-CONDITION.*\"severity\":\"warning\"")
add_test(NAME cli_vhdl_duplicate_condition COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/duplicate_condition.vhd")
set_tests_properties(cli_vhdl_duplicate_condition PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-DUPLICATE-CONDITION.*\"severity\":\"warning\"")
add_test(NAME cli_constant_expressions COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constant_expressions.sv")
set_tests_properties(cli_constant_expressions PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":8.*\"ruleId\":\"RTL-CONSTANT-CONDITION\",\"count\":8")
add_test(NAME cli_vhdl_constant_expressions COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/constant_expressions.vhd")
set_tests_properties(cli_vhdl_constant_expressions PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":8.*RTL-CONSTANT-CONDITION.*\"count\":8")
add_test(NAME cli_condition_side_effects COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/condition_side_effects.sv")
set_tests_properties(cli_condition_side_effects PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":7.*VL-SIDE-EFFECT-CONDITION.*\"count\":7")
add_test(NAME cli_empty_conditional_branches COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/empty_conditional_branches.sv")
set_tests_properties(cli_empty_conditional_branches PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":5.*VL-EMPTY-CONDITIONAL-BRANCH.*\"count\":5")
add_test(NAME cli_empty_case_items COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/empty_case_items.sv")
set_tests_properties(cli_empty_case_items PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":5.*VL-EMPTY-CASE-ITEM.*\"count\":5")
add_test(NAME cli_multiple_case_defaults COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/multiple_case_defaults.sv")
set_tests_properties(cli_multiple_case_defaults PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-MULTIPLE-CASE-DEFAULT.*\"severity\":\"error\"")
add_test(NAME cli_vhdl_multiple_case_defaults COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/multiple_case_defaults.vhd")
set_tests_properties(cli_vhdl_multiple_case_defaults PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*RTL-MULTIPLE-CASE-DEFAULT.*\"severity\":\"error\"")
add_test(NAME cli_case_width_mismatch COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/case_width_mismatch.sv")
set_tests_properties(cli_case_width_mismatch PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":4.*RTL-CASE-ITEM-WIDTH-MISMATCH.*\"count\":4")
add_test(NAME cli_vhdl_case_width_mismatch COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/case_width_mismatch.vhd")
set_tests_properties(cli_vhdl_case_width_mismatch PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*RTL-CASE-ITEM-WIDTH-MISMATCH.*\"count\":2")
add_test(NAME cli_case_value_out_of_range COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/case_value_out_of_range.sv")
set_tests_properties(cli_case_value_out_of_range PROPERTIES
    PASS_REGULAR_EXPRESSION "VL-CASE-ITEM-VALUE-OUT-OF-RANGE.*\"count\":4")
add_test(NAME cli_literal_width_truncation COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/literal_width_truncation.sv")
set_tests_properties(cli_literal_width_truncation PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":3.*RTL-WIDTH-TRUNCATION.*\"count\":3")
add_test(NAME cli_vhdl_literal_width_truncation COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/literal_width_truncation.vhd")
set_tests_properties(cli_vhdl_literal_width_truncation PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*RTL-WIDTH-TRUNCATION.*\"count\":2")
add_test(NAME cli_selected_target_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/selected_target_width.sv")
set_tests_properties(cli_selected_target_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*RTL-WIDTH-TRUNCATION.*\"count\":2")
add_test(NAME cli_vhdl_selected_target_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/selected_target_width.vhd")
set_tests_properties(cli_vhdl_selected_target_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*RTL-WIDTH-TRUNCATION.*\"count\":2")
add_test(NAME cli_selected_source_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/selected_source_width.sv")
set_tests_properties(cli_selected_source_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*RTL-WIDTH-TRUNCATION.*\"count\":2")
add_test(NAME cli_vhdl_selected_source_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/selected_source_width.vhd")
set_tests_properties(cli_vhdl_selected_source_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":2.*RTL-WIDTH-TRUNCATION.*\"count\":2")
add_test(NAME cli_concatenation_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/concatenation_width.sv")
set_tests_properties(cli_concatenation_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":3.*RTL-WIDTH-TRUNCATION.*\"count\":3")
add_test(NAME cli_vhdl_concatenation_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/concatenation_width.vhd")
set_tests_properties(cli_vhdl_concatenation_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":4.*RTL-WIDTH-TRUNCATION.*\"count\":4")
add_test(NAME cli_comparison_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/comparison_width.sv")
set_tests_properties(cli_comparison_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":15.*RTL-COMPARISON-WIDTH-MISMATCH.*\"count\":15")
add_test(NAME cli_vhdl_comparison_width COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/comparison_width.vhd")
set_tests_properties(cli_vhdl_comparison_width PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":10.*RTL-COMPARISON-WIDTH-MISMATCH.*\"count\":10")
add_test(NAME cli_shift_amounts COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/shift_amounts.sv")
set_tests_properties(cli_shift_amounts PROPERTIES
    PASS_REGULAR_EXPRESSION "RTL-SHIFT-AMOUNT-OUT-OF-RANGE.*\"count\":4")
add_test(NAME cli_vhdl_shift_amounts COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/shift_amounts.vhd")
set_tests_properties(cli_vhdl_shift_amounts PROPERTIES
    PASS_REGULAR_EXPRESSION "RTL-SHIFT-AMOUNT-OUT-OF-RANGE.*\"count\":5")
add_test(NAME cli_negative_shifts COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/negative_shifts.sv")
set_tests_properties(cli_negative_shifts PROPERTIES
    PASS_REGULAR_EXPRESSION "RTL-NEGATIVE-SHIFT-AMOUNT.*\"count\":4")
add_test(NAME cli_vhdl_negative_shifts COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/negative_shifts.vhd")
set_tests_properties(cli_vhdl_negative_shifts PROPERTIES
    PASS_REGULAR_EXPRESSION "RTL-NEGATIVE-SHIFT-AMOUNT.*\"count\":3")
add_test(NAME cli_divide_by_zero COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/divide_by_zero.sv")
set_tests_properties(cli_divide_by_zero PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":4.*RTL-DIVIDE-BY-ZERO.*\"severity\":\"error\"")
add_test(NAME cli_vhdl_divide_by_zero COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/divide_by_zero.vhd")
set_tests_properties(cli_vhdl_divide_by_zero PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":3.*RTL-DIVIDE-BY-ZERO.*\"severity\":\"error\"")
add_test(NAME cli_always_no_suspension COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/always_no_suspension.sv")
set_tests_properties(cli_always_no_suspension PROPERTIES
    PASS_REGULAR_EXPRESSION "VL-ALWAYS-NO-SUSPENSION.*\"count\":2")
add_test(NAME cli_cdc_synchronizer COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cdc_synchronizer.sv")
set_tests_properties(cli_cdc_synchronizer PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":0")
add_test(NAME cli_cdc_unsafe_bypass COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cdc_unsafe_bypass.sv")
set_tests_properties(cli_cdc_unsafe_bypass PROPERTIES
    PASS_REGULAR_EXPRESSION "\"violations\":1.*CDC-DIRECT-CROSSING.*\"severity\":\"warning\"")
add_test(NAME cli_vhdl_timing_split_safe COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_timing_split_safe")
set_tests_properties(cli_vhdl_timing_split_safe PROPERTIES
    PASS_REGULAR_EXPRESSION "\"filesAnalyzed\":2.*\"violations\":0.*\"clocks\":2.*\"sequentialProcesses\":2.*\"cdcSynchronizers\":1")
add_test(NAME cli_vhdl_timing_split_unsafe COMMAND fpga_analyzer_cli --format json --fail-on none
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/vhdl_timing_split_unsafe")
set_tests_properties(cli_vhdl_timing_split_unsafe PROPERTIES
    PASS_REGULAR_EXPRESSION "\"filesAnalyzed\":2.*\"violations\":1.*CDC-DIRECT-CROSSING.*\"severity\":\"warning\"")
if(FPGA_ANALYZER_BUILD_GUI)
    add_executable(fpga_gui_smoke gui_smoke.cpp $<TARGET_OBJECTS:fpga_branding>)
    target_link_libraries(fpga_gui_smoke PRIVATE fpga_gui fpga_build_options)
    add_test(NAME gui_smoke COMMAND fpga_gui_smoke -platform offscreen)
    set_tests_properties(gui_smoke PROPERTIES TIMEOUT 15)
    add_test(NAME gui_project_workflow COMMAND fpga_gui_smoke -platform offscreen)
    set_tests_properties(gui_project_workflow PROPERTIES TIMEOUT 25
        ENVIRONMENT "FPGA_GUI_TEST_PROJECT=${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
endif()
