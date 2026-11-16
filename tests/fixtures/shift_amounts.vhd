entity shift_amounts is
  port (data : in unsigned(7 downto 0); q : out unsigned(7 downto 0));
end entity;
architecture rtl of shift_amounts is begin
  process(all) begin
    q <= data sll 8;
    q <= data srl 7;
    q <= data(3 downto 0) sll 4;
    q(0) <= data(0) sra 1;
    q <= shift_left(data, 8);
    q <= shift_right(data, 7);
    q(3 downto 0) <= shift_left(data(3 downto 0), 4);
  end process;
end architecture;
