entity split_generic_fixture is
  generic (USED : positive := 2; UNUSED : positive := 1);
  port (d : in bit_vector(USED-1 downto 0); q : out bit);
end entity;
