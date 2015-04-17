
assertInterpolation({
  property: 'outline-width',
  from: '0px',
  to: '10px'
}, [
  {at: -0.3, is: '0px'}, // CSS outline-width can't be negative.
  {at: 0, is: '0px'},
  {at: 0.3, is: '3px'},
  {at: 0.6, is: '6px'},
  {at: 1, is: '10px'},
  {at: 1.5, is: '15px'}
]);
assertInterpolation({
  property: 'outline-width',
  from: 'thick',
  to: '15px'
}, [
  {at: -2, is: '0px'}, // CSS outline-width can't be negative.
  {at: -0.3, is: '2px'},
  {at: 0, is: '5px'},
  {at: 0.3, is: '8px'},
  {at: 0.6, is: '11px'},
  {at: 1, is: '15px'},
  {at: 1.5, is: '20px'}
]);
