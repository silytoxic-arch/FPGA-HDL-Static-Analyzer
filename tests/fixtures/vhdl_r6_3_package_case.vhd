library ieee;
use ieee.numeric_std.all;

package body helpers is
    procedure choose is
        variable selector : unsigned(7 downto 0);
        constant narrow : unsigned(3 downto 0) := (others => '0');
    begin
        case selector is
            when narrow => null;
            when others => null;
        end case;
    end procedure;
end package body;
