entity literal_width_truncation is
  port (q0, q1, q2 : out bit_vector(7 downto 0));
end entity;
architecture rtl of literal_width_truncation is begin
  process(all) begin
    q0 <= "101010101010";
    q1 <= (("111111111"));
    q2 <= "00000000";
  end process;
end architecture;
