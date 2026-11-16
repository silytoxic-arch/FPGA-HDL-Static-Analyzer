entity constant_condition is
  port (clk, d : in bit; q : out bit);
end entity;

architecture rtl of constant_condition is
begin
  worker : process(clk)
  begin
    if clk'event and clk = '1' then
      if false then
        q <= d;
      end if;
      while (true) loop
        exit;
      end loop;
      q <= d;
    end if;
  end process;
end architecture;
