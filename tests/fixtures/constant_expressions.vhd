entity constant_expressions is
  port (d : in std_logic; enable : in boolean; q, r : out std_logic);
end entity;
architecture rtl of constant_expressions is begin
  process(all) begin
    q <= '0';
    if false then q <= d; end if;
    while (true) loop
      exit;
    end loop;
    if enable and false then q <= d; end if;
    if true or enable then q <= d; end if;
    if not (2 = 2) then q <= d; end if;
    if enable or false then q <= d; end if;
    if d = '1' then q <= d; elsif false then q <= d; end if;
    q <= d when false else '0';
    q <= d when enable else '0';
  end process;
  r <= d when true else '0';
end architecture;
