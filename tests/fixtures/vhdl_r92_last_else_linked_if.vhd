-- R-9-2 boundary: the native category-1 ELSE loop keeps only the last direct
-- linked source IF. The first linked IF clock must not enter this rule's set.
entity r92_last_else_linked_if is
  port(clk1, clk2, rst, d : in bit; q1, q2, q3 : out bit);
end entity;

architecture rtl of r92_last_else_linked_if is
begin
  source_proc: process(clk1, clk2, rst)
  begin
    if rst = '0' then
      q1 <= '0';
    else
      if rising_edge(clk1) then
        q1 <= d;
      end if;
      if rising_edge(clk2) then
        q1 <= not d;
      end if;
    end if;
  end process;

  first_control_proc: process(clk1, d)
  begin
    if clk1 then
      q2 <= d;
    end if;
  end process;

  last_control_proc: process(clk2, d)
  begin
    if clk2 then
      q3 <= d;
    end if;
  end process;
end architecture;
