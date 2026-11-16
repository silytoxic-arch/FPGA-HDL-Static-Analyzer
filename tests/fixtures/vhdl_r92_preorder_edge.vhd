-- R-9-2 positive: the native relation builder selects the first edge found
-- in AST preorder, including a legacy edge nested below a logical root.
entity r92_preorder_edge is
  port(clk, en, d : in bit; q1, q2 : out bit);
end entity;

architecture rtl of r92_preorder_edge is
begin
  source_proc: process(clk, en)
  begin
    if (clk = '1' and clk'event) and en then
      q1 <= d;
    end if;
  end process;

  control_proc: process(clk, d)
  begin
    if clk then
      q2 <= d;
    end if;
  end process;
end architecture;
