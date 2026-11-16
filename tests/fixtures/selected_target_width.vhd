entity selected_target_width is
  port (idx : in integer; q0, q1, q2, q3 : out bit_vector(7 downto 0));
end entity;
architecture rtl of selected_target_width is begin
  process(all) begin
    q0(3 downto 0) <= "11111111";
    q1(0) <= "10";
    q2(3 downto 0) <= "1111";
    q3(idx) <= "10";
  end process;
end architecture;
