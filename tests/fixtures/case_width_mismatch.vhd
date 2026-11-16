entity case_width_mismatch is
  port (sel : in std_logic_vector(3 downto 0); q : out std_logic);
end entity;
architecture rtl of case_width_mismatch is begin
  process(all) begin
    case sel is
      when "000" => q <= '0';
      when "0001" => q <= '1';
      when "00010" => q <= '0';
      when others => q <= '0';
    end case;
    case sel(2 downto 0) is
      when "00" => q <= '0';
      when "001" => q <= '1';
      when others => q <= '0';
    end case;
    case sel(0) is
      when "00" => q <= '0';
      when "1" => q <= '1';
      when others => q <= '0';
    end case;
  end process;
end architecture;
