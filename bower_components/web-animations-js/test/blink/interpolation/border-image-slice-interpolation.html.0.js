
assertInterpolation({
  property: 'border-image-slice',
  from: '0%',
  to: '50%',
}, [
  {at: -0.3, is: '0%'}, // CSS border-image-slice can't be negative.
  {at: 0, is: '0%'},
  {at: 0.1, is: '5%'},
  {at: 0.2, is: '10%'},
  {at: 0.3, is: '15%'},
  {at: 0.4, is: '20%'},
  {at: 0.5, is: '25%'},
  {at: 0.6, is: '30%'},
  {at: 0.7, is: '35%'},
  {at: 0.8, is: '40%'},
  {at: 0.9, is: '45%'},
  {at: 1, is: '50%'},
  {at: 1.5, is: '75%'},
  {at: 10, is: '500%'}
]);
