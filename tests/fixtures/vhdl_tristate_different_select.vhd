entity tristate_different_select is
    port (a : in bit_vector(1 downto 0); c : inout bit);
end entity;

architecture rtl of tristate_different_select is
begin
    c <= a(0) when a(1) = '1' else 'Z';
end architecture;
