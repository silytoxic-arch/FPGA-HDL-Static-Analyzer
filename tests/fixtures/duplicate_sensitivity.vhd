entity duplicate_sensitivity is
  port (a, b : in bit; q : out bit);
end entity;

architecture rtl of duplicate_sensitivity is
begin
  comb : process(a, A, b)
  begin
    q <= a and b;
  end process;
end architecture;
