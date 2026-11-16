entity multiple_case_defaults is
  port (sel : in std_logic; q : out std_logic);
end entity;
architecture rtl of multiple_case_defaults is begin
  process(all) begin
    case sel is
      when '0' => q <= '0';
      when others => q <= '1';
      when others => q <= '0';
    end case;
  end process;
end architecture;
