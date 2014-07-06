lambda = [ 1 3 0.5 2];
U = [ 1 3 -1 4; 2 0.3 1 -2; -1 -2 -0.4 1; -0.9 1 1 2 ];
if ( det( U ) ~= 0 )
    P = U*diag(lambda)*inv(U)
else
    disp( 'U Non invertibile!' );
end