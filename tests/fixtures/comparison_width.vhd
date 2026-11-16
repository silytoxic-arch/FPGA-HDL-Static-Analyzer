entity comparison_width is
  port (a, c : in bit_vector(3 downto 0); b : in bit_vector(7 downto 0);
        en : in bit; q : out bit);
end entity;
architecture rtl of comparison_width is begin
  process(all) begin
    q <= '0';
    if a = b then q <= '1'; end if;
    if a /= c then q <= '1'; end if;
    if en = '1' and a(3 downto 2) = b(1 downto 0) then q <= '1'; end if;
    if a(1 downto 0) /= b(3 downto 0) then q <= '1'; end if;
    if (a & c) = b then q <= '1'; end if;
    if a = "00000000" then q <= '1'; end if;
    if a = "0000" then q <= '1'; end if;
    if a = (c & c) then q <= '1'; end if;
    if a < b then q <= '1'; end if;
    if a <= c then q <= '1'; end if;
    if a(1 downto 0) >= b(3 downto 0) then q <= '1'; end if;
    if (a & c) > b then q <= '1'; end if;
    while a < b loop
      q <= '1'; exit;
    end loop;
    while a(1 downto 0) >= b(3 downto 0) loop
      q <= '1'; exit;
    end loop;
    if not (a = b) then q <= '1'; end if;
    if not (a = c) then q <= '1'; end if;
    q <= '1' when a < b else '0';
    q <= '1' when a <= c else '0';
  end process;
end architecture;
