entity concatenation_width is
  port (a : in bit_vector(3 downto 0); b : in bit_vector(1 downto 0); idx : in integer;
        q0, q2 : out bit_vector(3 downto 0); q1 : out bit_vector(5 downto 0);
        q3 : out bit_vector(1 downto 0));
end entity;
architecture rtl of concatenation_width is begin
  process(all) begin
    q0 <= a & b;
    q1 <= a & b;
    q2 <= ("01" & b);
    q3 <= a(1 downto 0) & b;
    q3 <= a(1 downto 0) & '0';
    q3 <= a(1 downto 0) & 'X';
    q2 <= a(2 downto 0) & '0';
    q2 <= a(idx) & b;
  end process;
end architecture;
