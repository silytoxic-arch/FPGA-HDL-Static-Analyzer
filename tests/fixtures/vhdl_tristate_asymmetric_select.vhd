entity tristate_asymmetric_select is
    port (a : in bit_vector(0 downto 0); c : inout bit);
end entity;

architecture rtl of tristate_asymmetric_select is
begin
    c <= a(0) when a = "1" else 'Z';
end architecture;
