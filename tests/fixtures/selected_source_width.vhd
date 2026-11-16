entity selected_source_width is
  port (data : in bit_vector(15 downto 0); idx : in integer;
        q0, q1, q2 : out bit_vector(3 downto 0); q3 : out bit_vector(1 downto 0));
end entity;
architecture rtl of selected_source_width is begin
  process(all) begin
    q0 <= data(7 downto 0);
    q1 <= data(3 downto 0);
    q2 <= data(idx downto idx - 3);
    q3(1 downto 0) <= data(7 downto 4);
  end process;
end architecture;
