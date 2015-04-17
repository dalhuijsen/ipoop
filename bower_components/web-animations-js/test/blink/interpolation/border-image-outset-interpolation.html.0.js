
assertInterpolation({
  property: 'border-image-outset',
  from: '0px',
  to: '5px',
}, [
  {at: -0.3, is: '0px'}, // CSS border-image-outset can't be negative.
  {at: 0, is: '0px'},
  {at: 0.1, is: '0.5px'},
  {at: 0.2, is: '1px'},
  {at: 0.3, is: '1.5px'},
  {at: 0.4, is: '2px'},
  {at: 0.5, is: '2.5px'},
  {at: 0.6, is: '3px'},
  {at: 0.7, is: '3.5px'},
  {at: 0.8, is: '4px'},
  {at: 0.9, is: '4.5px'},
  {at: 1, is: '5px'},
  {at: 1.5, is: '7.5px'},
  {at: 10, is: '50px'}
]);
assertInterpolation({
  property: 'border-image-outset',
  from: '0',
  to: '5',
}, [
  {at: -0.3, is: '0'}, // CSS border-image-outset can't be negative.
  {at: 0, is: '0'},
  {at: 0.1, is: '0.5'},
  {at: 0.2, is: '1'},
  {at: 0.3, is: '1.5'},
  {at: 0.4, is: '2'},
  {at: 0.5, is: '2.5'},
  {at: 0.6, is: '3'},
  {at: 0.7, is: '3.5'},
  {at: 0.8, is: '4'},
  {at: 0.9, is: '4.5'},
  {at: 1, is: '5'},
  {at: 1.5, is: '7.5'},
  {at: 10, is: '50'}
]);
